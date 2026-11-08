#include "JitFunction.h"
#include "MemoryFunction.h"

#if defined(_WIN32) && defined(_M_X64)
#include "X86Assembler.h"

// Generated blocks use XMM6-XMM15 as scratch registers, including when they
// tail-chain to other blocks. Preserve the native Windows ABI once at the
// entry boundary rather than on every block in the chain.
static const CMemoryFunction& GetWin64InvokeTrampoline()
{
	static const CMemoryFunction trampoline = []() {
		using A = CX86Assembler;
		Framework::CMemStream stream;
		A a;
		a.SetStream(&stream);
		a.Begin();
		a.MarkLabel(a.CreateLabel());
		// 32-byte shadow space, ten 128-bit registers, and 8-byte alignment.
		a.SubIq(A::MakeRegisterAddress(A::rSP), 200);
		for(unsigned reg = 6; reg < 16; reg++)
		{
			a.MovdquVo(A::MakeIndRegOffAddress(A::rSP, 32 + (reg - 6) * 16), static_cast<A::XMMREGISTER>(reg));
		}
		a.MovEq(A::rAX, A::MakeRegisterAddress(A::rCX));
		a.MovEq(A::rCX, A::MakeRegisterAddress(A::rDX));
		a.CallEd(A::MakeRegisterAddress(A::rAX));
		for(unsigned reg = 6; reg < 16; reg++)
		{
			a.MovdquVo(static_cast<A::XMMREGISTER>(reg), A::MakeIndRegOffAddress(A::rSP, 32 + (reg - 6) * 16));
		}
		a.AddIq(A::MakeRegisterAddress(A::rSP), 200);
		a.Ret();
		a.End();
		return CMemoryFunction(stream.GetBuffer(), stream.GetSize());
	}();
	return trampoline;
}
#endif

void ExecuteJitFunction(CMemoryFunction& function, void* context)
{
#if defined(_WIN32) && defined(_M_X64)
	using InvokeType = void (*)(void*, void*);
	auto invoke = reinterpret_cast<InvokeType>(GetWin64InvokeTrampoline().GetCode());
	invoke(function.GetCode(), context);
#else
	function(context);
#endif
}
