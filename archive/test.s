	.text
	.file	"test.c"
	.globl	mod                             // -- Begin function mod
	.p2align	2
	.type	mod,@function
mod:                                    // @mod
// %bb.0:
	sdiv	w8, w0, w1
	msub	w0, w8, w1, w0
	ret
.Lfunc_end0:
	.size	mod, .Lfunc_end0-mod
                                        // -- End function
	.ident	"clang version 11.0.0 (https://github.com/termux/termux-packages 39dec01e591687a324c84205de6c9713165c4802)"
	.section	".note.GNU-stack","",@progbits
