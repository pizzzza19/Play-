#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
#include "Jitter_CodeGen_x86_64.h"
#include "MemoryFunction.h"
#include "JitFunction.h"
#include "MemStream.h"

// Generate a native caller as well as the JIT callee, so the test does not
// depend on which nonvolatile XMM registers the C++ compiler happens to use.
static CMemoryFunction* testCallee = nullptr;
static void InvokeTestCallee(void* context)
{
	ExecuteJitFunction(*testCallee, context);
}

int main()
{
#if defined(_WIN32) && defined(_M_X64)
	using A = CX86Assembler;
	for(bool avx : {false, true})
	{
		if(avx && !CX86CpuFeatures::AutoDetect().hasAvx) continue;
		for(bool scalar : {false, true})
			for(bool chain : {false, true})
				for(unsigned stackSize : {0U, 64U})
				{
					CX86CpuFeatures features;
					features.hasAvx = avx;
					Jitter::CCodeGen_x86_64 gen(features);
					gen.SetPlatformAbi(Jitter::CCodeGen_x86_64::PLATFORM_ABI_WIN32);
					Framework::CMemStream body;
					gen.SetStream(&body);
					Jitter::StatementList statements;
					std::vector<Jitter::SymbolPtr> symbols;
					auto ref = [&](Jitter::SYM_TYPE type, unsigned value) {
						auto symbol = std::make_shared<Jitter::CSymbol>(type, value, 0);
						symbols.push_back(symbol);
						return std::make_shared<Jitter::CSymbolRef>(symbol);
					};
					for(unsigned reg = 0; reg < 12; reg++)
					{
						Jitter::STATEMENT s;
						s.op = Jitter::OP_MOV;
						s.dst = ref(scalar ? Jitter::SYM_FP_REGISTER32 : Jitter::SYM_REGISTER128, reg);
						s.src1 = ref(scalar ? Jitter::SYM_FP_RELATIVE32 : Jitter::SYM_RELATIVE128, 0);
						statements.push_back(s);
					}
					Jitter::STATEMENT store;
					store.op = Jitter::OP_MOV;
					store.dst = ref(scalar ? Jitter::SYM_FP_RELATIVE32 : Jitter::SYM_RELATIVE128, 192);
					store.src1 = ref(scalar ? Jitter::SYM_FP_REGISTER32 : Jitter::SYM_REGISTER128, 11);
					statements.push_back(store);
					gen.GenerateCode(statements, stackSize);
					CMemoryFunction callee(body.GetBuffer(), body.GetSize());
					testCallee = &callee;
					std::unique_ptr<CMemoryFunction> chainedCallee;
					if(chain)
					{
						auto address = reinterpret_cast<uintptr_t>(callee.GetCode());
						auto symbol = std::make_shared<Jitter::CSymbol>(Jitter::SYM_CONSTANTPTR, static_cast<uint32>(address), static_cast<uint32>(address >> 32));
						symbols.push_back(symbol);
						Jitter::STATEMENT jump;
						jump.op = Jitter::OP_EXTERNJMP;
						jump.src1 = std::make_shared<Jitter::CSymbolRef>(symbol);
						statements.push_back(jump);
						Framework::CMemStream chainBody;
						gen.SetStream(&chainBody);
						gen.GenerateCode(statements, stackSize);
						chainedCallee = std::make_unique<CMemoryFunction>(chainBody.GetBuffer(), chainBody.GetSize());
						testCallee = chainedCallee.get();
					}

					Framework::CMemStream caller;
					A a;
					a.SetStream(&caller);
					a.Begin();
					a.MarkLabel(a.CreateLabel());
					a.Push(A::rBX);
					a.MovEq(A::rBX, A::MakeRegisterAddress(A::rCX));
					a.SubIq(A::MakeRegisterAddress(A::rSP), 192);
					for(unsigned reg = 6; reg < 16; reg++)
					{
						auto xmm = static_cast<A::XMMREGISTER>(reg);
						a.MovdquVo(A::MakeIndRegOffAddress(A::rSP, 32 + (reg - 6) * 16), xmm);
						a.MovdquVo(xmm, A::MakeIndRegOffAddress(A::rBX, 16));
					}
					a.MovIq(A::rAX, reinterpret_cast<uintptr_t>(&InvokeTestCallee));
					a.CallEd(A::MakeRegisterAddress(A::rAX));
					for(unsigned reg = 6; reg < 16; reg++)
					{
						auto xmm = static_cast<A::XMMREGISTER>(reg);
						a.MovdquVo(A::MakeIndRegOffAddress(A::rBX, 32 + (reg - 6) * 16), xmm);
						a.MovdquVo(xmm, A::MakeIndRegOffAddress(A::rSP, 32 + (reg - 6) * 16));
					}
					a.AddIq(A::MakeRegisterAddress(A::rSP), 192);
					a.Pop(A::rBX);
					a.Ret();
					a.End();
					CMemoryFunction probe(caller.GetBuffer(), caller.GetSize());
					alignas(16) std::array<unsigned char, 208> context{};
					std::memset(context.data(), 0xCC, 16);
					for(unsigned i = 0; i < 16; i++)
						context[16 + i] = static_cast<unsigned char>(i * 13 + 7);
					probe(context.data());
					if(std::memcmp(context.data(), context.data() + 192, scalar ? 4 : 16)) return 2;
					for(unsigned reg = 6; reg < 16; reg++)
					{
						if(std::memcmp(context.data() + 16, context.data() + 32 + (reg - 6) * 16, 16))
						{
							std::printf("FAIL %s stack=%u XMM%u was clobbered\n", avx ? "AVX" : "SSE", stackSize, reg);
							return 1;
						}
					}
					std::printf("PASS %s %s %s stack=%u XMM6-XMM15 preserved\n", avx ? "AVX" : "SSE", scalar ? "scalar" : "vector", chain ? "tail-chain" : "direct", stackSize);
				}
	}
#endif
	return 0;
}
