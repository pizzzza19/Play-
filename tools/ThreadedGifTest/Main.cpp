#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <chrono>
#include "AppConfig.h"
#include "PathUtils.h"
#include "MIPS.h"
#include "FrameDump.h"
#include "ee/DMAC.h"
#include "ee/GIF.h"
#include "gs/GSH_Null.h"
#include "uint128.h"

fs::path CAppConfig::GetBasePath() const
{
	auto path = fs::current_path() / "threaded-gif-test-data";
	Framework::PathUtils::EnsurePathExists(path);
	return path;
}

class CaptureGs : public CGSH_Null
{
public:
	std::vector<RegisterWrite> writes;
	std::thread::id consumerThread;
	explicit CaptureGs(bool threaded = true)
	{
		if(!threaded) m_threadedGif = false;
	}
	bool IsThreadedGifEnabled() const
	{ return m_threadedGif; }

protected:
	void WriteRegisterImpl(uint8 reg, uint64 value) override
	{
		consumerThread = std::this_thread::get_id();
		writes.emplace_back(reg, value);
	}
	void ResetImpl() override
	{ writes.clear(); }
};

static void Verify(bool ok, const char* message)
{
	if(!ok) throw std::runtime_error(message);
}

struct Fixture
{
	CaptureGs gs;
	CGSHandler* gsPointer = &gs;
	std::vector<uint8> memory = std::vector<uint8>(16384);
	CMIPS context{MEMORYMAP_ENDIAN_LSBF};
	CDMAC dmac{memory.data(), memory.data(), memory.data(), memory.data(), context};
	CGIF gif{gsPointer, dmac, memory.data(), memory.data()};
	explicit Fixture(bool threaded = true)
	    : gs(threaded)
	{
		gs.Initialize();
		gif.Reset();
	}
	~Fixture()
	{
		gs.Finish(true);
		gs.Release();
	}
	void Run(std::vector<uint128> data, uint32 chunk)
	{
		gs.Reset();
		gif.Reset();
		auto bytes = static_cast<uint32>(data.size() * 16);
		auto input = reinterpret_cast<uint8*>(data.data());
		for(uint32 address = 0; address < bytes;)
		{
			auto consumed = gif.ProcessSinglePacket(input, bytes, address, std::min(bytes, address + chunk), CGsPacketMetadata(1));
			Verify(consumed != 0, "GIF made no progress");
			// Producers may reuse the source immediately after the transfer.
			memset(input + address, 0xA5, consumed);
			address += consumed;
		}
		Verify(gif.GetActivePath() == 0, "GIF path did not retire");
		gs.Finish(true);
	}
};

static std::vector<uint128> Packet(uint64 regs, unsigned count, unsigned loops)
{
	std::vector<uint128> data(1 + count * loops);
	CGIF::TAG tag{};
	tag.loops = loops;
	tag.eop = 1;
	tag.pre = 1;
	tag.prim = 3;
	tag.nreg = count & 15;
	tag.regs = regs;
	memcpy(data.data(), &tag, 16);
	uint32 state = 0x12345678;
	for(size_t i = 1; i < data.size(); i++)
	{
		for(auto& value : data[i].nV)
		{
			state ^= state << 13;
			state ^= state >> 17;
			state ^= state << 5;
			value = state;
		}
	}
	return data;
}

int main()
{
#ifdef DEBUGGER_INCLUDED
	puts("SKIP: packed GIF deferral is disabled for frame dumps");
	return 0;
#else
	try
	{
		Fixture serial(false);
		Fixture threaded;
		unsigned cases = 0;
		const unsigned descriptors[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 13, 15};
		for(unsigned count = 1; count <= 16; count++)
		{
			uint64 regs = 0;
			for(unsigned reg = 0; reg < count; reg++)
				regs |= static_cast<uint64>(descriptors[reg % 13]) << (reg * 4);
			for(unsigned chunk : {16U, 32U, 80U, 4096U})
			{
				auto data = Packet(regs, count, 7);
				serial.Run(data, chunk);
				threaded.Run(data, chunk);
				Verify(serial.gs.writes == threaded.gs.writes, "Packed register sequence mismatch");
				cases++;
			}
		}
		// A+D events must keep their immediate producer-side behavior.
		auto events = Packet(14, 1, 3);
		events[1].nD0 = 0xFFFFFFFF12345678ULL;
		events[1].nD1 = GS_REG_SIGNAL;
		events[2].nD0 = 0;
		events[2].nD1 = GS_REG_FINISH;
		events[3].nD0 = 0xFFFFFFFF87654321ULL;
		events[3].nD1 = GS_REG_LABEL;
		serial.Run(events, 4096);
		threaded.Run(events, 4096);
		Verify(serial.gs.writes == threaded.gs.writes, "A+D order mismatch");
		Verify(serial.gs.ReadPrivRegister(CGSHandler::GS_CSR) == threaded.gs.ReadPrivRegister(CGSHandler::GS_CSR), "Guest event state mismatch");
		// Invalid/high guest addresses cannot be mistaken for internal pointers.
		events = Packet(14, 1, 1);
		events[1].nD1 = 255;
		serial.Run(events, 4096);
		threaded.Run(events, 4096);
		Verify(serial.gs.writes == threaded.gs.writes, "Guest register tag collided with queue type");
		// Force the GS consumer behind the producer, then reuse frame slots.
		serial.gs.Reset();
		threaded.gs.Reset();
		threaded.gs.SendGSCall([]() { std::this_thread::sleep_for(std::chrono::milliseconds(30)); });
		for(unsigned frame = 0; frame < 100; frame++)
		{
			for(auto fixture : {&serial, &threaded})
			{
				fixture->gif.Reset();
				auto data = Packet(0x512, 3, 37);
				data[1].nV0 ^= frame;
				auto bytes = static_cast<uint32>(data.size() * 16);
				Verify(fixture->gif.ProcessSinglePacket(reinterpret_cast<uint8*>(data.data()), bytes, 0, bytes, CGsPacketMetadata(1)) == bytes, "Frame stress transfer failed");
				memset(data.data(), 0xCC, bytes);
				fixture->gs.Finish();
			}
		}
		serial.gs.Finish(true);
		threaded.gs.Finish(true);
		Verify(serial.gs.writes == threaded.gs.writes, "In-flight frame storage was overwritten");
		// SaveState must retire a batch smaller than the submission threshold.
		threaded.gs.Reset();
		threaded.gif.Reset();
		auto pending = Packet(0x512, 3, 3);
		auto bytes = static_cast<uint32>(pending.size() * 16);
		threaded.gif.ProcessSinglePacket(reinterpret_cast<uint8*>(pending.data()), bytes, 0, bytes, CGsPacketMetadata(1));
		Framework::CZipArchiveWriter archive;
		threaded.gs.SaveState(archive);
		Verify(threaded.gs.writes.size() == 10, "SaveState left packed writes pending");
		// Switching GS backends must also retire a short pending batch before
		// copying registers and RAM out of the old handler.
		threaded.gs.Reset();
		threaded.gif.Reset();
		threaded.gif.ProcessSinglePacket(reinterpret_cast<uint8*>(pending.data()), bytes, 0, bytes, CGsPacketMetadata(1));
		{
			Fixture replacement;
			replacement.gs.Copy(&threaded.gs);
			Verify(threaded.gs.writes.size() == 10, "Backend copy left packed writes pending");
		}
		Verify(!threaded.gs.QueuePackedGif(pending.data(), 16 * 1024 * 1024 / 16 + 1, 0x512, 3, 0, 0), "Queue exceeded its frame storage bound");
		Verify(!threaded.gs.QueuePackedGif(pending.data(), 1, 14, 1, 0, 0), "A+D was incorrectly admitted");
		{
			Fixture normal;
			normal.Run(Packet(0x512, 3, 3), 4096);
			Verify(normal.gs.IsThreadedGifEnabled(), "Default GUI mode did not enable threaded GIF");
		}
		Verify(threaded.gs.consumerThread != std::this_thread::get_id(), "Writes did not run on the separate GS thread");
		printf("PASS %u packed/split cases and 100 lagged frames; immutable input, save fence, A+D event order, guest-address isolation, separate GS thread\n", cases);
		return 0;
	}
	catch(const std::exception& error)
	{
		fprintf(stderr, "FAIL %s\n", error.what());
		return 1;
	}
#endif
}
