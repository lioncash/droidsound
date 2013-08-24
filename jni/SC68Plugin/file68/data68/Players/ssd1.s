;;; sidsound designer sc68 relocator
;;; 
;;; by Benjamin Gerard <http://sourceforge.net/users/benjihan>
;;;
;;; Time-stamp: <2011-09-12 15:18:50 ben>
;;;
;;; More info on sc68 ssd1 in file custom/ssd1-sc68.s

	include	"lib/org.s"
	
	bra	init
	bra	player+4
	bra	player+8
	
;; something gor wrong with relocation ... probably as68 screwing up.

if 0
{
init:	movem.l	d0/d1/a0/a1,-(a7)
	lea	offset(pc),a0
	lea	player(pc),a1
	bsr.s	reloc
	movem.l	(a7)+,d0/d1/a0/a1
	bra	player

reloc:	incbin	"custom/reloc.bin"
offset:	incbin	"custom/ssd1-sc68.rel"
player:	incbin	"custom/ssd1-sc68.bin"
}
player:	include	"custom/ssd1-sc68.s"
