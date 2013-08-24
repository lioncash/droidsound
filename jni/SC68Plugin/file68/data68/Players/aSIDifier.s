;
; aSIDifier(tm)
; 
; Add SID synthesis to classic music.
;
; by Benjamin Gerard <https://sourceforge.net/users/benjihan>
;
; Time-stamp: <2013-08-13 21:04:57 ben>
;
; TODO:
;	- TONE_MODE as an option instead of conditionnal assembly
; 

; Set this to 1 to get some new sound FX !
TONE_MODE = 0
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Header:
;;;
;;;  Classic sc68/sndh (...) music driver header.
;;; 
	bra	init
	bra	exit
	bra	play
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; DO NOT CHANGE PLACE AND ORDER
	
music_ptr:
	dc.l	0		; Pointer to current music replay
aSid_activ:
	print	aSid_activ
	dc.w	0      ; $0000:off $FFFF:on $00FF:enable $FF00:disable

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Init code:
;;; 
;;; - init aSIDfier
;;; - save MFP
;;; - run music replay init code.
;;; 
;;; IN:
;;; 
;;;  specific to aSIDifier:
;;;	a6: original music replay [+0 init, +4 stop, +8 play]
;;;	d7: timer selection. Use token like 'abcx' where a,b,c,x is
;;;	    one of 'A','B','C' or 'D' the timer to use for respectively
;;;           canal a,b,c. 'x' is the unused timer and *MUST* be set too.
;;;	    Default is 'ACDB'.
;;;
;;;  specific to sc68:
;;;	a0: music data (for external replay)
;;;	d0: sub-song number
;;;	d1: ste-selection (0:stf, 1:ste)
;;;	d2: music data size (size of buffer pointed by a0)
;;; 
;;; OUT:
;;; 	none, all registers restored 
	
init:
	movem.l	d0-a6,-(a7)

	;; Clear some buffers	
	movem.l	d0/a0,-(a7)

	;; Clear timer
	lea	timers(pc),a0
	move.w	#timer_sz*4,d0
	bsr	clear

	;; Clear voice info
	lea	ym_status(pc),a0
	moveq	#ym_sz,d0
	bsr	clear

	movem.l	(a7)+,d0/a0
	
	;; init timer tables
	bsr	init_timers
	
	;; compute timer table
	bsr	compute_timer_table

	;; compute period to chord table
	bsr	compute_per2chd_table
	
	;; save MFP
	bsr	save_mfp
	
	;; call music init 
	lea	music_ptr(pc),a5
	move.l	a6,(a5)
	jsr	(a6)

	;; set active to enable
  	lea	aSid_activ(pc),a0
	move.w	#$00ff,(a0)
	
	;;  reset ym status buffer
	bsr	get_ym_status

	;; setup voice info structs
	lea	ym_status(pc),a0
	move.w	d7,ym_m7(a0)
	movem.w	d0-d1,ym_A+ym_per(a0)
	movem.w	d2-d3,ym_B+ym_per(a0)
	movem.w	d4-d5,ym_C+ym_per(a0)
	moveq	#2,d0
.lp_voice:
	st	ym_chd+1(a0)
	move.w	#3,ym_cct(a0)
	move.l	ym_tim(a0),a1
	moveq	#10,d1
	sub	d0,d1
	ror.l	#8,d1
	move.l	d1,timer_volH(a1)
	move.l	d1,timer_volL(a1)
	lea	ym_vsz(a0),a0
	dbf	d0,.lp_voice
	
	movem.l	(a7)+,d0-a6
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; 
;;; Exit code:
;;; 
;;; - Run music stop code
;;; - Stop aSIDifier
;;; - restore MFP
;;; - cut YM
;;;
;;; IN:	none
;;; OUT:	none
;;; 
exit:
	movem.l	d0-a6, -(a7)
	
	lea	music_ptr(pc),a6
	move.l	(a6),d0
	beq.s	.exit

	;; clear music pointer and aSid status
	clr.l	(a6)+
	clr.w	(a6)+
	
	;; call music stop
	move.l	d0,a6
	jsr	4(a6)

	;; restore mfp
	bsr	restore_mfp
	
	;; cut YM
	lea	$ffff8800.w,a0
	move.l	#$08000000,(a0)
	move.l	#$09000000,(a0)
	move.l	#$0A000000,(a0)
	
.exit:
	movem.l	(a7)+,d0-a6
	rts
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;  
;;; Play code:
;;;
play:
	;; check music ptr
	move.l	music_ptr(pc),d0
	bne.s	.running
	rts

.running:
	;; What to do depends on aSid_activ value.
	lea	aSid_activ(pc),a1
	move.w	(a1),d7
	bne.s	.not_off

	;; aSid_activ=$0000 -> aSid OFF
.asid_off:	
	move.l	d0,a0
	jmp	8(a0)
	
.not_off:
	cmp.w	#-1,d7
	bne.s	.changed
	
	;; aSid_activ=$FFFF -> aSid ON
.asid_on:
	bsr.s	ym_restore
.no_restore:
	move.l	d0,a0
	jsr	8(a0)
	move	sr,-(a7)
	move	#$2700,sr
	bsr	aSidifier
	move	(a7)+,sr
	rts
	
.changed:
	tst.b	d7
	beq.s	.disable
	
	;; aSid_activ=$00FF -> enable aSid
.enable:
	move	#$FFFF,(a1)
	bra.s	.no_restore

	;; aSid_activ=$FF00 -> disable aSid
.disable:
	clr.w	(a1)
	bsr.s	disable_timers
	bsr.s	ym_restore
	bra.s	.asid_off

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Disable timers:
;;;
;;;   Disable timer interruptions on aSidfied voices
	
disable_timers:
	movem.l	d0-d2/a0-a2,-(a7)
	move	sr,-(a7)
	move	#$2700,sr
	
	;; clear intena and intmsk for each voice timer
	moveq	#3-1,d2
	lea	ym_status+ym_A(pc),a2
	lea	$fffffa00.w,a1
.lp_voice:
	move.l	ym_tim(a2),a0
	moveq	#$7,d1
	moveq	#0,d0
	move.b	timer_channel(a0),d0
	and.b	d0,d1			; d0 = bit number
	lsr	#3,d0			; d0 is channel [0/2]
	bclr	d1,$07(a1,d0)
	bclr	d1,$13(a1,d0)
	lea	ym_vsz(a2),a2
	dbf	d2,.lp_voice

	move	(a7)+,sr
	movem.l	(a7)+,d0-d2/a0-a2
	rts


;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; YM restore:
;;;
;;;   Restore YM registers modified by aSid (7,8,9,A).
	
ym_restore:
	move.l	a1,-(a7)
	move	d7,-(a7)
	move	sr,-(a7)
	
	;; Restore mixer register but port-A and port-B
	move	#$2700,sr
	move	#%11000000,d7
	lea	$ffff8800.w,a1
	move.b	#7,(a1)
	and.b	(a1),d7
	or.w	ym_status+ym_m7(pc),d7
	move.b	d7,2(a1)
	move.w	ym_status+ym_A+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	move.w	ym_status+ym_B+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	move.w	ym_status+ym_C+ym_vol(pc),d7
  	movep.w	d7,0(a1)
	
	move.w	(a7)+,sr
	move	(a7)+,d7
	move.l	(a7)+,a1
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;;
;;; Init timer:
;;;
;;;    - Init timers table
;;;    - Assign timers to voices
;;; 
;;; In:  d7: timer assignment ('ACDB')
	
init_timers:
	movem.l	d0-a6,-(a7)

	;; scan requested timers
	moveq	#0,d6
	moveq	#3,d1
.lp_test:	
	rol.l	#8,d7
	sub.b	#'A',d7
	moveq	#3,d5
	and.b	d7,d5
	bset	d5,d6
	dbf	d1,.lp_test

	cmp.b	#15,d6
	beq.s	.ok

	;; all timers were not set properly, reset to default
	move.l	#00020301,d7
.ok:
	;; copy timer info struct
	lea	timer_def_table(pc),a0
	lea	timers(pc),a1
	moveq	#3,d1
.lp_copy:
	move.l	(a0)+,(a1)+
	move.l	(a0)+,(a1)+
	move.w	(a0)+,(a1)+
	lea	timer_sz-10(a1),a1
	dbf	d1,.lp_copy

	;; assign timer to voice info
	lea	ym_status+ym_A(pc),a0
	lea	timers(pc),a1
	moveq	#2,d1
.lp_assign:
	rol.l	#8,d7
	moveq	#3,d5
	and.b	d7,d5
	mulu	#timer_sz,d5
	lea	0(a1,d5),a2
	move.l	a2,ym_tim(a0)
	lea	ym_vsz(a0),a0
	dbf	d1,.lp_assign

	movem.l	(a7)+,d0-a6
	rts

; clear memory block
; 
; IN:
; 	a0: memory to clear
;	d0: number of bytes (word)
; 
clear:
	subq.w	#1,d0
.clr:
	clr.b	(a0)+
	dbf	d0,.clr
	rts
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

	RSRESET
timersv_vector:	rs.w	1
timersv_irq:	rs.l	1
timersv_ctrlr:	rs.w	1
timersv_mask:	rs.b	1
timersv_ctrl:	rs.b	1
timersv_sz:	rs.b	0	
	
	RSRESET
mfp_timer0:	rs.b	timersv_sz
mfp_timer1:	rs.b	timersv_sz
mfp_timer2:	rs.b	timersv_sz
mfp_timer3:	rs.b	timersv_sz
mfp_intena:	rs.w	1
mfp_intmsk:	rs.w	1
mfp_17:		rs.w	1
mfp_size:	rs.b	0	

; MFP save buffer
mfp_buffer:
	ds.b	mfp_size
	even
	
; Save MFP interruption.
; 
save_mfp:
	movem.l	d0-d1/a0-a3,-(a7)
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	mfp_buffer(pc),a0
	lea	$fffffa00.w,a1
	lea	timers(pc),a2

	moveq	#0,d0
	moveq	#3,d1
.loop:		
	move.w	timer_vector(a2),a3
	move.w	a3,(a0)+
	move.l	(a3),(a0)+
	move.w	timer_ctrlreg(a2),a3
	move.w	a3,(a0)+
	move.b	timer_mask(a2),d0
	move.b	d0,(a0)+
	and.b	(a3),d0
	move.b	d0,(a0)+
	lea	timer_sz(a2),a2
	dbf	d1,.loop
	
	movep.w	$07(a1),d0
	and.w	#$2130,d0
	move.w	d0,(a0)+	; enable bits
	movep.w	$13(a1),d0
	and.w	#$2130,d0
	move.w	d0,(a0)+	; mask bits
	move.b	$17(a1),(a0)+	; SEI

	move	(a7)+,sr
	movem.l	(a7)+,d0-d1/a0-a3
	
	rts

; Restore MFP interruption
; 
restore_mfp:
	movem.l	d0-d1/a0-a4,-(a7)
	
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	mfp_buffer(pc),a0
	lea	$fffffa00.w,a1

	;; cut intena for timers
	movep.w	$07(a1),d0
	and.w	#~$2130,d0
	movep.w	d0,$07(a1)

	moveq	#0,d0
	moveq	#2,d1		; Only restore 3 used timers !
.loop:
	move.w	(a0)+,a2	; timer vector
	move.l	(a0)+,a3	; saved vector
	move.w	(a0)+,a4	; control reg
	move.b	(a0)+,d0	; saved mask
	not.b	d0		; invert to mask other
	and.b	(a4),d0
	or.b	(a0)+,d0
	move.b	d0,(a4)		; restore control reg
	move.l	a3,(a2)		; restore vector
	dbf	d1,.loop

	lea	timersv_sz(a0),a0 ; skip 4th timer

	;; Restore intena
	movep.w	$07(a1),d0
	and.w	#~$2130,d0
	or.w	(a0)+,d0
	movep.w	d0,$07(a1)
	
	;; Restore intmsk
	movep.w	$13(a1),d0
	and.w	#~$2130,d0
	or.w	(a0)+,d0
	movep.w	d0,$13(a1)

	;; Restore SEI/AEI 
	move.b	(a0)+,$17(a1)

	move	(a7)+,sr
	movem.l	(a7)+,d0-d1/a0-a4

	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; Asidifier private data
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

	RSRESET
	
ym_per:	rs.w	1		; period
ym_vol:	rs.w	1		; volume
ym_cyr:	rs.w	1		; cyclic ratio
ym_cy2:	rs.w	1		; cyclic ratio 2
ym_crs:	rs.w	1		; cyclic ratio step
ym_cr2:	rs.w	1		; cyclic ratio 2 step
ym_lat:	rs.w	1		; aSID latch
ym_chd:	rs.w	1		; current chord
ym_cct:	rs.w	1		; change chord counter
ym_tim:	rs.l	1		; pointer to timer struct
ym_vsz:	rs.w	0

	RSRESET
ym_A:	rs.b	ym_vsz
ym_B:	rs.b	ym_vsz
ym_C:	rs.b	ym_vsz
ym_m7:	rs.w	1
ym_sz:	rs.w	0

ym_status:
	ds.b	ym_sz


;;; Get YM stat
;;; 
;;; output:
;;;
;;; d0,d1: voice-A per,vol
;;; d2,d3: voice-B per,vol
;;; d4,d5: voice-C per,vol
;;; d7:    mixer
get_ym_status:
	lea	$ffff8800.w,a0

	;; Get voice A
	moveq	#$0f,d0
	move.b	#$1,(a0)
	and.b	(a0),d0
	
	lsl	#8,d0
	move.b	#$0,(a0)
	or.b	(a0),d0
	
	move	#$081f,d1
	move.b	#$08,(a0)
	and.b	(a0),d1

	;; Get voice B
	moveq	#$0f,d2
	move.b	#$3,(a0)
	and.b	(a0),d2
	
	lsl	#8,d2
	move.b	#$2,(a0)
	or.b	(a0),d2
	
	move	#$091f,d3
	move.b	#$09,(a0)
	and.b	(a0),d3

	;; Get voice C
	moveq	#$0f,d4
	move.b	#$5,(a0)
	and.b	(a0),d4
	
	lsl	#8,d4
	move.b	#$4,(a0)
	or.b	(a0),d4
	
	move	#$0a1f,d5
	move.b	#$0a,(a0)
	and.b	(a0),d5

	;; Get mixer stat
	move.w	#$073f,d7
	move.b	#$7,(a0)
	and.b	(a0),d7
		
	rts

;;; ------------------------------------------------------------
;;; ------------------------------------------------------------
;;; aSIDifier (o_O)
;;; 
  		
aSidifier:

	;; Read and store current ym status
	bsr	get_ym_status
	lea	ym_status(pc),a6
	movem.w	d0-d1,ym_A+ym_per(a6)
	movem.w	d2-d3,ym_B+ym_per(a6)
	movem.w	d4-d5,ym_C+ym_per(a6)
	move.w	d7,ym_m7(a6)

;;; mixer rules:
;;; -----------
;;; noise | tone | aSIDdifier
;;; 0       0      0
;;; 0       1      0
;;; 1       0      1
;;; 1       1      0
;;;
;;; aSIDifier = (noise^tone)&noise

NOISE_LATCH   = 6
NOSOUND_LATCH = 3
BUZZ_LATCH    = 3

	moveq	#0,d6
	moveq	#0,d2
	moveq	#0,d0
	lea	ym_A(a6),a5
	lea	per2chd(pc),a4
.loop_latch:

	;; Test noise / sound / envelop and set latch

	move	d7,d5
	lsr	d6,d5

	btst	#3,d5
	bne.s	.nonoise
	moveq	#NOISE_LATCH,d5
	bra.s	.set_latch
.nonoise:
	btst	#0,d5
	beq.s	.sound
	moveq	#NOSOUND_LATCH,d5
	bra.s	.set_latch
	
.sound:
 	btst	#4,ym_vol+1(a5)
 	beq.s	.nobuzz
 	moveq	#BUZZ_LATCH,d5
 	bra.s	.set_latch
.nobuzz:
	move	ym_lat(a5),d5
	beq.s	.aSIDactiv
	subq.w	#1,d5

	;; Set new latch
.set_latch:
 	move.w	d5,ym_lat(a5)
	beq.s	.aSIDactiv
	
	;; Not activ, reset some stuff
	st.b	ym_chd+1(a5)		; invalid chord
	clr.l	ym_cyr(a5)		; reset cyr(s)
	move.w	#6,ym_cct(a5)		; fake counter will force chord reload
	bra.s	.next_latch
	
.aSIDactiv:	
	;; Chord change detect

	move	ym_per(a5),d2		; d2: new period
	move.b	0(a4,d2),d0		; d0: new chord
	bsr	get_chord_period	; d1: new period
	sub	d1,d2			; d2: delta period
	subx	d3,d3
	eor	d3,d2
	sub.w	d3,d2			; d2: |delta period|
	
	move	d0,d3
	lsr	#4,d3			; octave 0-8
	addq	#4,d3
	lsl	d3,d2
	add	#$411,d2

	move	d2,ym_cr2(a5)

	if (TONE_MODE)
	{
 	move	ym_per(a5),d2
 	add	d2,d1		; d2+d1
 	add	d2,d2		; d2+d2
 	add	d2,d1		; 3*d2+d1
 	lsr	#2,d1

	move	d1,ym_per(a5)	; try use true note timer period
; 	clr.w	ym_css(a5)
	}


	cmp.w	ym_chd(a5),d0
	
	beq.s	.no_chord_change
	
.chord_change:
	move	d0,d2
	move	d0,ym_chd(a5)		; store new chord
	moveq	#15,d1
	and	d0,d1
	lsr.b	#4,d0
	add.b	d0,d1
	add	d1,d1
	move.w	chord_stp(pc,d1),d1

; 	move	#$800,d1
; 	asr	#1,d1

	move.w	ym_cct(a5),d3
	subq	#1,d3
	cmp.w	#24,d3
	bhi.s	.arpeggio
    	move.b	d2,ym_cyr(a5)		; reset cyclic ratio
  	clr.b	ym_cyr+1(a5)
;     	clr.w	ym_cy2(a5)		; reset cyclic ratio 2
	
.arpeggio:
	move.w	d1,ym_crs(a5)
	clr.w	ym_cct(a5)
	
.no_chord_change:
	addq.w	#1,ym_cct(a5)
	

.next_latch:
	lea	ym_vsz(a5),a5
	addq	#1,d6
	cmp.w	#3,d6
	bne	.loop_latch

	;; reset aSIDfied voice
	moveq	#0,d7

	;; voice A
	moveq	#0,d6
	lea	ym_status+ym_A(pc),a6
	bsr	aSIDvoicify

	;; voice B
	moveq	#1,d6
	lea	ym_status+ym_B(pc),a6
	bsr	aSIDvoicify

	;; voice C
	moveq	#2,d6
	lea	ym_status+ym_C(pc),a6
	bsr	aSIDvoicify

	;; Cut aSIDdified voice in mixer, keep port-A and port-B bits

	if (!TONE_MODE)
	{
	move	sr,-(a7)
	move	#$2700,sr
	
	lea	$fffff8800.w,a6
	move.b	#7,(a6)
	moveq	#%11000000,d6
	and.b	(a6),d6
  	and	#%111,d7
  	or	ym_status+ym_m7(pc),d7
 	or.b	d7,d6
     	move.b	d6,2(a6)
	
	move	(a7)+,sr
	}
	
	rts

CHORD_STP_MIN	=	$300
CHORD_STP_MSK	=	$3FF

chord_step:	MACRO
	{
	dc.w	((\1&CHORD_STP_MSK)+CHORD_STP_MIN)*(1-(\2&2))
	}
	
RND_1	=	0
RND_2	=	$17299283
	
chord_stp:
	RPT	16
	{
	chord_step	RND_1,RND_1
RND_1	=	RND_1+RND_2
	
	}
		

;;; aSIDify one voice.
;;; 
;;; IN:
;;; 
;;;	d6: num voice
;;;	a6: voice struct
;;;	d7: current SIDfied flags
;;; 
;;; OUT:
;;;	d7: new SIDfied flags
;;; 
aSIDvoicify:
	
	;; Get timer table
	move.l	ym_tim(a6),a2
	lea	timer_irq_base(pc),a3
	add	timer_irq(a2),a3
	
	tst.w	ym_lat(a6)
	bne	.noA

	;; period/volume
	movem.w	ym_per(a6),d0-d1

	and.b	#$1f,d1
	beq	.noA
	cmp.w	#$6,d0
	ble	.noA

	;; Increment cyclic ratio
	lea	sinus(pc),a5
	movem.w	ym_cyr(a6),d2-d5 ; cyr,cyr2,crs,cr2
	
	add	d5,d3
    	add	d4,d2
	
	move	d3,d5
	lsr	#6,d5
	add	d5,d5
	move	0(a5,d5),d5
	asr	#4,d5
 	add	d5,d2
	
	movem.w	d2-d3,ym_cyr(a6)
	move	d2,d5
	move	d3,d4
	
	;; timer control/data
	move	d0,d3
	add	d3,d3
	lea	Tper2MFP(pc),a5
	move.w	0(a5,d3),d2	; ctrl+data
	beq	.noA		; sorry, not available for this period 

	;; control reg
	move	d2,d3
	lsr	#8,d3
	and.b	timer_mask(a2),d3	; d3: control reg

; 	add	ym_cyr(a6),d5
	

	if (1)
	{
; 	add	d4,d5
	lsr	#7,d5		; 0-512

; 	lsr	#7,d4		; 0-512
; 	add	d4,d5		; 0-1024
	
	add	d5,d5
	lea	sinus(pc),a5
	move.w	0(a5,d5.w),d5
	add	#$1000,d5
	}

;   	bpl.s	.okpos
;   	neg.w	d5
	
;  .okpos:
  	if (TONE_MODE)
  	{
 	sub.w	#$C000,d5
;    	neg.w	d5
  	}
;  	if (!TONE_MODE)
;  	{
;       	add	d5,d5
;  	}

	;; data regs (d2:hi / d4:lo)
	and.w	#255,d2
	subq.b	#1,d2		; 1->0, 2->1, 0->255
	addq.w	#1,d2		; [V] [1..256]
	move	d2,d4		; [N]
	mulu	d5,d2
	
	swap	d2		; [V]
	tst.w	d2
	bne.s	.not_empty
	moveq	#0,d1		;  set volume to 0
	move	d4,d2
	add	d4,d4		;  use a trick, get d2 = d4 after next sub
.not_empty:
 	sub	d2,d4
	
	;; Set volumes
 	move.b	d1,timer_volH+2(a2)	; set volume HI
 	clr.b	timer_volL+2(a2)	; set volume LO

	;; Set data reg in routines
	move.b	d2,timer_dataH(a2)
	move.b	d4,timer_dataL(a2)

	;; Start timer
	move.w	timer_ctrlreg(a2),a5	; control reg addrx
	move.b	(a5),d4			; d4: ctrl all
	move.b	d4,d5

	moveq	#0,d0
	move.b	timer_mask(a2),d0	; d0: mask other
	and.w	d0,d4			; d4: me
	not.b	d0			; d0: mask me
	and.w	d0,d5			; d5: other
	
	cmp.b	d4,d3
	beq.s	.no_progA

	move.b	d5,(a5)			; stop
	move.w	timer_vector(a2),a1	; vector addr
	move.l	a3,(a1)			; Set vector
	move.w	timer_datareg(a2),a1	; data reg addr
 	move.b	d2,(a1)			; start with data hi
	or.b	d3,d5	
	move.b	d5,(a5)			; GO timer ! GO !
	bra.s	.ok_progA
	
.no_progA:
	;; Not programmed, we should take care to retrieve good level
	move.w	(a2),a1		; vector addr
	move.l	(a1),d5		; current routine

	sub.l	a3,d5		; 0:next routine is HI, current is LO
	sne	d5
	and.b	d5,d1
	
	moveq	#8,d2
	add.b	d6,d2
	swap	d2
	move.b	d1,d2
	lsl.l	#8,d2
	move.l	d2,$ffff8800.w
	
.ok_progA:		
	;; set intena and intmsk
	moveq	#$7,d0
	moveq	#0,d1
	move.b	timer_channel(a2),d1
	and.b	d1,d0			; d0 = bit number
	lsr	#3,d1			; d1 is channel [0/2]
	lea	$fffffa00.w,a1
	bset	d0,$07(a1,d1)
	bset	d0,$13(a1,d1)
	bset	#3,$17(a1)		; set MFP to AEI
	bset	d6,d7
	
	rts
	
.noA:
	bclr	d6,d7
	move.w	timer_ctrlreg(a2),a5	; control reg addr
	move.b	timer_mask(a2),d0	; d0: mask other
	not.b	d0			; d0: mask me
	and.b	d0,(a5)			; stop timer
	rts


; timer info struct (aligned to 32 bytes)
	RSRESET
timer_vector:	rs.w	1	; vector address
timer_ctrlreg:	rs.w	1	; timer control register
timer_datareg:	rs.w	1	; timer data register
timer_mask:	rs.b	1	; bit used in ctrl reg
timer_channel:	rs.b	1	; MFP channel and bit
timer_irq:	rs.w	1	; offset from timer_irq_base
timer_volH:	rs.l	1	; Value to set YM HI
timer_volL:	rs.l	1	; Value to set YM LO
timer_dataH:	rs.b	1	; data register value for HI
timer_dataL:	rs.b	1	; data register value for LO
timer_sz:	rs.w	0
	
; vector, ctrl-reg, data-reg, msk.b+chan.q+bit.q
timer_def_table:
tAdef:	dc.w	$134, $fa19, $fa1f, $0f05, timerA_irq-timer_irq_base
tBdef:	dc.w	$120, $fa1b, $fa21, $0f00, timerB_irq-timer_irq_base
tCdef:	dc.w	$114, $fa1d, $fa23, $f015, timerC_irq-timer_irq_base
tDdef:	dc.w	$110, $fa1d, $fa25, $0f14, timerD_irq-timer_irq_base

timers:
timerA:	ds.b	timer_sz
timerB:	ds.b	timer_sz
timerC:	ds.b	timer_sz
timerD:	ds.b	timer_sz
	
;;; ------------------------------------------------------------
;;; Timer interruption routines
;;; - name (A/B/C) referes to the sound channel not the timer
;;; ------------------------------------------------------------


	
;;; \1:	'A','B','C','D'
;;; \2:	timer vector
;;; \3:	timer data reg
timerN:	MACRO
	{
timer\1_irq:
timer\1_irq_H:
	move.l	timer\1+timer_volH(pc),$ffff8800.w
	pea	timer\1_irq_L(pc)
	move.l	(a7)+,\2
 	move.b	timer\1+timer_dataL(pc),\3
	rte
	
timer\1_irq_L:
	move.l	timer\1+timer_volL(pc),$ffff8800.w
	pea	timer\1_irq_H(pc)
	move.l	(a7)+,\2
 	move.b	timer\1+timer_dataH(pc),\3
	rte
	}

	dc.w	$1234
timer_irq_base:	
	timerN	A,$134.w,$fffffa1f.w
	timerN	B,$120.w,$fffffa21.w
	timerN	C,$114.w,$fffffa23.w
	timerN	D,$110.w,$fffffa25.w
	
;;; ------------------------------------------------------------
;;; ------------------------------------------------------------

;;; per = 125000/frq
;;; frq = 125000/per
;;; frq = 2457600/(       unsigned int frq = 8000000*192 / t->cpp; */


;;; timer_fmin = 2457600/(200*256)	; 48 Hz
;;; timer_fmax = 2457600/(4*1)		; 614400 Hz

;;; frq = 125000/per
;;; frq = 2457600/width
;;; 125000 / p = 2457600 / w
;;; w * 125000 / p = 2457600
;;; w = 2457600 * p / 125000
;;; w = 12288 * p / 625	
;;; w = d * r
;;; d * r = 12288 * p / 625
;;; d = 12288 * p / (625*r)

Tper2MFP:
	ds.w	$1000
	
;;; Timer prediviser table
timer_prediv:
	dc.w	0*625, 4/2*625, 10/2*625, 16/2*625
	dc.w	50/2*625, 64/2*625, 100/2*625, 200/2*625
	
compute_timer_table:
	movem.l	d0-d3/a0-a1,-(a7)
	
	lea	Tper2MFP(pc),a0
	lea	timer_prediv+2(pc),a1
	move.w	#$1100,d1
	moveq	#0,d3
	moveq	#1,d0		; skip period 0 :)
 	clr	(a0)+
.lp:
	mulu	#12288/2,d0
.retry:		
	move.l	d0,d2
	divu	(a1),d2
	cmp.w	#256,d2
	ble.s	.ok
.advance:
	addq	#2,a1
	add.w	#$1100,d1
	and.w	#$7700,d1
	bne.s	.retry
	
.clear:
	clr.w	(a0)+
	addq	#1,d3
	cmp.w	#$1000,d3
	bne.s	.clear
	bra.s	.finish

.ok:	
	move.b	d2,d1
	move.w	d1,(a0)+
	addq	#1,d3
	move.l	d3,d0
	
	cmp.w	#$1000,d3
	bne.s	.lp
	
.finish:
	movem.l	(a7)+,d0-d3/a0-a1
	rts

;;; IN:
;;;	d0 = chords.q octave.q
;;; OUT:
;;;	d1 = YM period
get_chord_period:
	moveq	#15,d1
	and	d0,d1
	add	d1,d1
	move.w	chords(pc,d1.w),d1
	ror	#4,d0
	lsr	d0,d1
	rol	#4,d0
	rts

;;; Chords table:
;;; - One octave at lowest frequency available for the YM tone generator
chords:	
	dc.w	$EEE,$E17,$D4D,$C8E,$BD9,$B2F,$A8E,$9F7,$967,$8E0,$861,$7E8
	dc.w	$EEE/2	
	
sinus:	
;
	dc.w	0,201,402,603,804,1005,1206,1406
	dc.w	1607,1808,2009,2209,2410,2610,2811,3011
	dc.w	3211,3411,3611,3811,4011,4210,4409,4608
	dc.w	4807,5006,5205,5403,5601,5799,5997,6195
	dc.w	6392,6589,6786,6982,7179,7375,7571,7766
	dc.w	7961,8156,8351,8545,8739,8932,9126,9319
	dc.w	9511,9703,9895,10087,10278,10469,10659,10849
	dc.w	11038,11227,11416,11604,11792,11980,12166,12353
	dc.w	12539,12724,12909,13094,13278,13462,13645,13827
	dc.w	14009,14191,14372,14552,14732,14911,15090,15268
	dc.w	15446,15623,15799,15975,16150,16325,16499,16672
	dc.w	16845,17017,17189,17360,17530,17699,17868,18036
	dc.w	18204,18371,18537,18702,18867,19031,19194,19357
	dc.w	19519,19680,19840,20000,20159,20317,20474,20631
	dc.w	20787,20942,21096,21249,21402,21554,21705,21855
	dc.w	22004,22153,22301,22448,22594,22739,22883,23027
	dc.w	23169,23311,23452,23592,23731,23869,24006,24143
	dc.w	24278,24413,24546,24679,24811,24942,25072,25201
	dc.w	25329,25456,25582,25707,25831,25954,26077,26198
	dc.w	26318,26437,26556,26673,26789,26905,27019,27132
	dc.w	27244,27355,27466,27575,27683,27790,27896,28001
	dc.w	28105,28208,28309,28410,28510,28608,28706,28802
	dc.w	28897,28992,29085,29177,29268,29358,29446,29534
	dc.w	29621,29706,29790,29873,29955,30036,30116,30195
	dc.w	30272,30349,30424,30498,30571,30643,30713,30783
	dc.w	30851,30918,30984,31049,31113,31175,31236,31297
	dc.w	31356,31413,31470,31525,31580,31633,31684,31735
	dc.w	31785,31833,31880,31926,31970,32014,32056,32097
	dc.w	32137,32176,32213,32249,32284,32318,32350,32382
	dc.w	32412,32441,32468,32495,32520,32544,32567,32588
	dc.w	32609,32628,32646,32662,32678,32692,32705,32717
	dc.w	32727,32736,32744,32751,32757,32761,32764,32766
;
	dc.w	32767,32766,32764,32761,32757,32751,32744,32736
	dc.w	32727,32717,32705,32692,32678,32662,32646,32628
	dc.w	32609,32588,32567,32544,32520,32495,32468,32441
	dc.w	32412,32382,32350,32318,32284,32249,32213,32176
	dc.w	32137,32097,32056,32014,31970,31926,31880,31833
	dc.w	31785,31735,31684,31633,31580,31525,31470,31413
	dc.w	31356,31297,31236,31175,31113,31049,30984,30918
	dc.w	30851,30783,30713,30643,30571,30498,30424,30349
	dc.w	30272,30195,30116,30036,29955,29873,29790,29706
	dc.w	29621,29534,29446,29358,29268,29177,29085,28992
	dc.w	28897,28802,28706,28608,28510,28410,28309,28208
	dc.w	28105,28001,27896,27790,27683,27575,27466,27355
	dc.w	27244,27132,27019,26905,26789,26673,26556,26437
	dc.w	26318,26198,26077,25954,25831,25707,25582,25456
	dc.w	25329,25201,25072,24942,24811,24679,24546,24413
	dc.w	24278,24143,24006,23869,23731,23592,23452,23311
	dc.w	23169,23027,22883,22739,22594,22448,22301,22153
	dc.w	22004,21855,21705,21554,21402,21249,21096,20942
	dc.w	20787,20631,20474,20317,20159,20000,19840,19680
	dc.w	19519,19357,19194,19031,18867,18702,18537,18371
	dc.w	18204,18036,17868,17699,17530,17360,17189,17017
	dc.w	16845,16672,16499,16325,16150,15975,15799,15623
	dc.w	15446,15268,15090,14911,14732,14552,14372,14191
	dc.w	14009,13827,13645,13462,13278,13094,12909,12724
	dc.w	12539,12353,12166,11980,11792,11604,11416,11227
	dc.w	11038,10849,10659,10469,10278,10087,9895,9703
	dc.w	9511,9319,9126,8932,8739,8545,8351,8156
	dc.w	7961,7766,7571,7375,7179,6982,6786,6589
	dc.w	6392,6195,5997,5799,5601,5403,5205,5006
	dc.w	4807,4608,4409,4210,4011,3811,3611,3411
	dc.w	3211,3011,2811,2610,2410,2209,2009,1808
	dc.w	1607,1406,1206,1005,804,603,402,201

; Currently not used
	if (0)
	{
;
	dc.w	0,-201,-402,-603,-804,-1005,-1206,-1406
	dc.w	-1607,-1808,-2009,-2209,-2410,-2610,-2811,-3011
	dc.w	-3211,-3411,-3611,-3811,-4011,-4210,-4409,-4608
	dc.w	-4807,-5006,-5205,-5403,-5601,-5799,-5997,-6195
	dc.w	-6392,-6589,-6786,-6982,-7179,-7375,-7571,-7766
	dc.w	-7961,-8156,-8351,-8545,-8739,-8932,-9126,-9319
	dc.w	-9511,-9703,-9895,-10087,-10278,-10469,-10659,-10849
	dc.w	-11038,-11227,-11416,-11604,-11792,-11980,-12166,-12353
	dc.w	-12539,-12724,-12909,-13094,-13278,-13462,-13645,-13827
	dc.w	-14009,-14191,-14372,-14552,-14732,-14911,-15090,-15268
	dc.w	-15446,-15623,-15799,-15975,-16150,-16325,-16499,-16672
	dc.w	-16845,-17017,-17189,-17360,-17530,-17699,-17868,-18036
	dc.w	-18204,-18371,-18537,-18702,-18867,-19031,-19194,-19357
	dc.w	-19519,-19680,-19840,-20000,-20159,-20317,-20474,-20631
	dc.w	-20787,-20942,-21096,-21249,-21402,-21554,-21705,-21855
	dc.w	-22004,-22153,-22301,-22448,-22594,-22739,-22883,-23027
	dc.w	-23169,-23311,-23452,-23592,-23731,-23869,-24006,-24143
	dc.w	-24278,-24413,-24546,-24679,-24811,-24942,-25072,-25201
	dc.w	-25329,-25456,-25582,-25707,-25831,-25954,-26077,-26198
	dc.w	-26318,-26437,-26556,-26673,-26789,-26905,-27019,-27132
	dc.w	-27244,-27355,-27466,-27575,-27683,-27790,-27896,-28001
	dc.w	-28105,-28208,-28309,-28410,-28510,-28608,-28706,-28802
	dc.w	-28897,-28992,-29085,-29177,-29268,-29358,-29446,-29534
	dc.w	-29621,-29706,-29790,-29873,-29955,-30036,-30116,-30195
	dc.w	-30272,-30349,-30424,-30498,-30571,-30643,-30713,-30783
	dc.w	-30851,-30918,-30984,-31049,-31113,-31175,-31236,-31297
	dc.w	-31356,-31413,-31470,-31525,-31580,-31633,-31684,-31735
	dc.w	-31785,-31833,-31880,-31926,-31970,-32014,-32056,-32097
	dc.w	-32137,-32176,-32213,-32249,-32284,-32318,-32350,-32382
	dc.w	-32412,-32441,-32468,-32495,-32520,-32544,-32567,-32588
	dc.w	-32609,-32628,-32646,-32662,-32678,-32692,-32705,-32717
	dc.w	-32727,-32736,-32744,-32751,-32757,-32761,-32764,-32766
;
	dc.w	-32767,-32766,-32764,-32761,-32757,-32751,-32744,-32736
	dc.w	-32727,-32717,-32705,-32692,-32678,-32662,-32646,-32628
	dc.w	-32609,-32588,-32567,-32544,-32520,-32495,-32468,-32441
	dc.w	-32412,-32382,-32350,-32318,-32284,-32249,-32213,-32176
	dc.w	-32137,-32097,-32056,-32014,-31970,-31926,-31880,-31833
	dc.w	-31785,-31735,-31684,-31633,-31580,-31525,-31470,-31413
	dc.w	-31356,-31297,-31236,-31175,-31113,-31049,-30984,-30918
	dc.w	-30851,-30783,-30713,-30643,-30571,-30498,-30424,-30349
	dc.w	-30272,-30195,-30116,-30036,-29955,-29873,-29790,-29706
	dc.w	-29621,-29534,-29446,-29358,-29268,-29177,-29085,-28992
	dc.w	-28897,-28802,-28706,-28608,-28510,-28410,-28309,-28208
	dc.w	-28105,-28001,-27896,-27790,-27683,-27575,-27466,-27355
	dc.w	-27244,-27132,-27019,-26905,-26789,-26673,-26556,-26437
	dc.w	-26318,-26198,-26077,-25954,-25831,-25707,-25582,-25456
	dc.w	-25329,-25201,-25072,-24942,-24811,-24679,-24546,-24413
	dc.w	-24278,-24143,-24006,-23869,-23731,-23592,-23452,-23311
	dc.w	-23169,-23027,-22883,-22739,-22594,-22448,-22301,-22153
	dc.w	-22004,-21855,-21705,-21554,-21402,-21249,-21096,-20942
	dc.w	-20787,-20631,-20474,-20317,-20159,-20000,-19840,-19680
	dc.w	-19519,-19357,-19194,-19031,-18867,-18702,-18537,-18371
	dc.w	-18204,-18036,-17868,-17699,-17530,-17360,-17189,-17017
	dc.w	-16845,-16672,-16499,-16325,-16150,-15975,-15799,-15623
	dc.w	-15446,-15268,-15090,-14911,-14732,-14552,-14372,-14191
	dc.w	-14009,-13827,-13645,-13462,-13278,-13094,-12909,-12724
	dc.w	-12539,-12353,-12166,-11980,-11792,-11604,-11416,-11227
	dc.w	-11038,-10849,-10659,-10469,-10278,-10087,-9895,-9703
	dc.w	-9511,-9319,-9126,-8932,-8739,-8545,-8351,-8156
	dc.w	-7961,-7766,-7571,-7375,-7179,-6982,-6786,-6589
	dc.w	-6392,-6195,-5997,-5799,-5601,-5403,-5205,-5006
	dc.w	-4807,-4608,-4409,-4210,-4011,-3811,-3611,-3411
	dc.w	-3211,-3011,-2811,-2610,-2410,-2209,-2009,-1808
	dc.w	-1607,-1406,-1206,-1005,-804,-603,-402,-201
	
	}

;;; Compute per2chd table.
;;; see per2chd for more info.
;;; 
compute_per2chd_table:
	movem.l	d0-a1,-(a7)

	lea	chords(pc),a0
	lea	per2chd+$1000(pc),a1

	moveq	#0,d1		; d1: octave
	moveq	#0,d2		; d2: chord in octave
	move.w	(a0)+,d3
	move	(a0)+,d4
	add	d4,d3
	lsr	#1,d3		; d3: limit
	move	#$fff,d7
.loop:
	cmp.w	d3,d7
	bge.s	.ok
	
	addq	#1,d2
	cmp	#12,d2
	bne.s	.same_octave
	
	lea	chords+2(pc),a0
	moveq	#0,d2
	addq	#1,d1

.same_octave:
	move.w	(a0)+,d3
	lsr	d1,d3
	exg	d4,d3
	add	d4,d3
	lsr	#1,d3
.ok:		
	move	d1,d0
	lsl.b	#4,d0
	or.b	d2,d0
	move.b	d0,-(a1)
	dbf	d7,.loop
		
	movem.l	(a7)+,d0-a1
	rts

;;; Table to convert YM periods to the nearest chord.
;;; Each value is $XY
;;; where:
;;; - X is octave in the range [$0..$9].
;;; - Y is chord in the range [$0..$b]
;;;
;;; note:
;;; 	YM periods = chords[Y]>>X
per2chd:
	ds.b	$1000
