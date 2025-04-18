
SynthDef.new(\tweet_01, { 
    Out.ar(0, n=LFNoise0.ar(_);f=[60,61];tanh(BBandPass.ar(max(max(n.(4),l=n.(6)),SinOsc.ar(f*ceil(l*9).lag(0.1))*0.7),f,n.(1).abs/2)*700*l.lag(1)))
}).writeDefFile(".");

SynthDef.new(\tweet_02, { 
    Out.ar(0, t=GaussTrig.ar(0.5);r=TRand.ar(0,1,t);e=EnvGen.ar(Env.sine(r*0.2+0.3),t);Pan2.ar(Gendy1.ar(minfreq:(r*3+1+(e*3))*300)*e,2*r-1))
}).writeDefFile(".");

SynthDef.new(\tweet_03, { 
    Out.ar(0, a=LFSaw;a.ar((b=a.ar(1/3))+1**a.ar(b)*(99+c=[0,1]))%a.ar(b*99,c)%a.ar(1/32)+a.ar(a.ar(b)*4e4%2e3,0,a.ar(6,c)>0.9/2)/2)
}).writeDefFile(".");

SynthDef.new(\tweet_04, { 
    Out.ar(0, n=99;{{|x|x=x+1;SinOsc.ar(x*n+(LFNoise2.ar(1)*x*XLine.ar(1,n,n)),0,0.5/x)}.dup(n).sum}!2)
}).writeDefFile(".");

SynthDef.new(\tweet_05, { 
    Out.ar(0, {a=SinOsc;l=LFNoise2;a.ar(666*a.ar(l.ar(l.ar(0.5))*9)*RLPF.ar(Saw.ar(9),l.ar(0.5).range(9,999),l.ar(2))).cubed}!2)
}).writeDefFile(".");

SynthDef.new(\tweet_06, { 
    Out.ar(0, a=LFSaw;Splay ar:HPF.ar(MoogFF.ar(a.ar(50*b=(0.999..9))-Blip.ar(a.ar(b)+9,b*99,9),a.ar(b/8)+1*999,a.ar(b/9)+1*2),9)/3)
}).writeDefFile(".");

SynthDef.new(\tweet_07, { 
    Out.ar(0, n=Duty;AllpassC.ar(LFTri.ar(n.kr(0.1,0,Dseq([999,99,4000],inf)),0,1)*Dust.kr(n.kr(1,0,Dseq([1,5],inf))),0.2,0.02,1)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_08, { 
    Out.ar(0, a=SinOsc.ar(LFNoise0.ar(10).range(100,1e4),0,0.05)*Decay.kr(Dust.kr(1));GVerb.ar(a*LFNoise1.ar(40),299,400,0.2,0.5,50,0,0.2,0.9))
}).writeDefFile(".");

SynthDef.new(\tweet_09, { 
    Out.ar(0, CombN.ar(SinOscFB.ar(Duty.ar(4,0,Dseq(44+[4,7,2,0],inf)).midicps*[1,1.5],LFNoise0.ar(8!2,1/3,1/2),1/8),1,1/2,9))
}).writeDefFile(".");

SynthDef.new(\tweet_10, { 
    Out.ar(0, AllpassC.ar(Splay.ar(tanh((LFNoise0.ar(8!2,8).lag(*{LFNoise2.ar(4).exprange(5e-3,500)}!2)*pi).cos*75),0.5),1,8e-3,LFNoise0.ar(8,2)))
}).writeDefFile(".");

SynthDef.new(\tweet_11, { 
    Out.ar(0, Array.fill(2,{Decay.ar(Pulse.ar(LFNoise0.ar(3+2.rand,4,5)),0.03, RHPF.ar(PinkNoise.ar,LFNoise2.kr(20,100,2000+3000.rand),0.5))});)
}).writeDefFile(".");

SynthDef.new(\tweet_12, { 
    Out.ar(0, r=Impulse;c=TChoose;a=(240..8000);n=c.kr(r.kr(2),a/920);PitchShift.ar(BPF.ar(LFNoise0.ar(8,0.5),c.kr(r.kr(n),a),0.5),n/33,n/2)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_13, { 
    Out.ar(0, Splay.ar({Pluck.ar(BPF.ar(f=product({|i|product({LFPulse.ar(2**2.rand2,2.rand/2)}!(i+2))/(1+i)+1}!8)*86,43).sin,Saw.ar,1,1/f,9)}!9))
}).writeDefFile(".");

SynthDef.new(\tweet_14, { 
    Out.ar(0, {x=LFNoise0.ar(1)>0;SinOsc.ar(Spring.ar(x,4,3e-05)*(70.rand+190)+(30.rand+90))*EnvGen.kr(Env.perc(0.001,5),x)}!2)
}).writeDefFile(".");

SynthDef.new(\tweet_15, { 
    Out.ar(0, l=LFNoise2;o=0.3;FreeVerb.ar(LPF.ar(SinOsc.ar(l.ar(o).range(666,1e3))*Saw.ar(17),300),l.ar(o))!2)
}).writeDefFile(".");

SynthDef.new(\tweet_16, { 
    Out.ar(0, x=[164,166];n=Spring;n.ar(LFTri.ar(x),SinOsc.ar(x.dup(5)*HenonC.ar(x).range(rrand(0.99,1.01),1)).range(-0.5,0.5),1e-3).clip2(1))
}).writeDefFile(".");

SynthDef.new(\tweet_17, { 
    Out.ar(0, x=LFSaw.ar([0.3,0.6])*LFSaw.ar([1,1.5])*SinOsc.ar([440,220]*LFPulse.kr(0.12).range(1,2));CombC.ar(x,0.5,0.5,9,0.5,x)/12)
}).writeDefFile(".");

SynthDef.new(\tweet_18, { 
    Out.ar(0, Pan2.ar(FreeVerb.ar(CombC.ar(LFTri.ar([440,220])*SinOsc.ar([XLine.kr(0.01,880,15),4])* Pulse.ar(0.5).range(1,2),0.9,0.9,10)))/10)
}).writeDefFile(".");

SynthDef.new(\tweet_19, { 
    Out.ar(0, SinOsc.ar([a=[1,8,2]]*96,0,Decay.kr(Demand.kr([b=Impulse.kr(a,[0,1,0.5])],0,Dseq([1,9,0,0],16))*b,0.1)).fold2(0.1)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_20, { 
    Out.ar(0, t=Stepper;w=Saw;LFGauss.ar(w.ar(a=t.ar(Dust.ar(0.5),0,1,8,1),0.001,w.ar(2,0.003,0.005)),(a+1.1)*0.02)*SinOsc.ar(10)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_21, { 
    Out.ar(0, Ringz.ar(Crackle.ar.tanh,2000.exprand(20),{0.6.rand}!2)/800*Decay.ar(TDuty.ar(Dseq([Dshuf((0,1..5)/6,3)],inf))))
}).writeDefFile(".");

SynthDef.new(\tweet_22, { 
    Out.ar(0, CombC.ar(Decay2.ar(Impulse.ar(1))*SinOsc.ar(800)*0.1,4,LFNoise2.ar(LFNoise1.kr(0.1).range(0.1,2)).range(0.01,3),10)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_23, { 
    Out.ar(0, a=Pulse;(40..61).midicps.clump(2).collect{|x,y|RHPF.ar(a.ar(x+Decay.ar(Impulse.ar(7/(y+3)),1/(y+2),x*4))/12,y*100+40,0.02)}.sum.tanh)
}).writeDefFile(".");

SynthDef.new(\tweet_24, { 
    Out.ar(0, GVerb.ar({|i|RLPF.ar(Saw.ar(1e3/(i+3),Decay.ar(Impulse.ar(1/(i+1),0.5),1)),500*i+1e3,0.4)}.dup(10).sum*0.2,77))
}).writeDefFile(".");

SynthDef.new(\tweet_25, { 
    Out.ar(0, j=Impulse;x=Demand.kr(j.kr(1/4),0,Dseq(0.3/[15,15,12,20],inf));({|i|Pluck.ar(Saw.ar(1200/(i+2)),j.ar(2/(i+4)),1,x,20)}!7).sum.sin!2)
}).writeDefFile(".");

SynthDef.new(\tweet_26, { 
    Out.ar(0, x=Decay2.ar(Impulse.ar([4,3]))*Blip.ar(50,4)+LocalIn.ar(2);LocalOut.ar(DelayC.ar(x,2,TChoose.kr(Impulse.kr(1/4),[3,4,6]/8).lag));x)
}).writeDefFile(".");

SynthDef.new(\tweet_27, { 
    Out.ar(0, x=LFNoise1.kr(1/4,7,8);GVerb.ar(Decay2.ar(Impulse.ar([x,x+1]),x/2e3,x/50)*Blip.ar(x*3+50,x*2),2,60))
}).writeDefFile(".");

SynthDef.new(\tweet_28, { 
    Out.ar(0, a=LFSaw;Splay.ar({[LFCub,a].choose.ar(20.rrand(99).midicps,mul:[LFPulse,a].choose.kr((1/9).exprand(11)))}!4.rrand(16)))
}).writeDefFile(".");

SynthDef.new(\tweet_29, { 
    Out.ar(0, mean({|j|i=j+1;CombC.ar(x=0.5**i*Pluck.ar(a=Saw.ar(1/i+1/6),a,1,1/i/(3-LFPulse.ar(1/i))/30,9,1b/i),1,SinOsc ar:i+[4,9]*1e-3,0)-x}!9)/9)
}).writeDefFile(".");

SynthDef.new(\tweet_30, { 
    Out.ar(0, GVerb.ar(Splay.ar(SinOsc.ar(0,Blip.ar(a=(1..5),99)*99,Blip.ar(a+2.5,a).lag2(LFSaw.ar(1/(a+2.25),2/a)+1)))/3,99,6,0.7))
}).writeDefFile(".");

SynthDef.new(\tweet_31, { 
    Out.ar(0, a=SinOsc;c=a.ar(0,a.ar(b=[2,3])*400,a.ar(b/4.1));c+a.ar(b*99*Amplitude.ar(c,0,1/7))+GrayNoise.ar(CombN.ar(c,1,b/3))/2)
}).writeDefFile(".");

SynthDef.new(\tweet_32, { 
    Out.ar(0, a=LFTri;BufWr.ar(a.ar([2.995,4]*99),b=LocalBuf(3e4,2).clear,a.ar([2,6]/99)*3e4);BufRd.ar(2,b,a.ar([6,9.06]/99)*9e3)/5)
}).writeDefFile(".");

SynthDef.new(\tweet_33, { 
    Out.ar(0, a=SinOscFB;c=a.ar([50,99],0.4);RecordBuf.ar(InFeedback.ar(0,2)+c/3,b=LocalBuf(8e4,2).clear);BufRd.ar(2,b,a.ar(c)*6e4))
}).writeDefFile(".");

SynthDef.new(\tweet_34, { 
    Out.ar(0, b=(1,3.075..16);a=SinOsc;GVerb.ar(Splay.ar(a.ar(1/b,3*a.ar(b*Duty.ar(b,0,Dseq(b+23,inf).midicps).lag(2))).tanh/5),90))
}).writeDefFile(".");

SynthDef.new(\tweet_35, { 
    Out.ar(0, FreeVerb.ar(SinOsc.ar((440*LFNoise1.ar(99).ceil.clip)+300*Pulse.ar(1/4+4*SinOsc.ar(2)),0,0.5),SinOsc.kr(0.1,0,0.1,0.2),[0.3,0.2]))
}).writeDefFile(".");

SynthDef.new(\tweet_36, { 
    Out.ar(0, SinOsc.ar(EnvGen.kr(Env.new([0,pi/3,pi],[5.3,7.1]))*Pulse.kr(LFSaw.kr(7),mul:0.7)*2200!2))
}).writeDefFile(".");

SynthDef.new(\tweet_37, { 
    Out.ar(0, Splay.arFill(8,{ SinOscFB.ar(LFNoise0.kr(1)*440,LFNoise0.kr(1))*LFPulse.ar(LFNoise0.kr(1)*8)}))
}).writeDefFile(".");

SynthDef.new(\tweet_38, { 
    Out.ar(0, l=LFDNoise3;PMOsc.ar([l.kr(0.06).range(10,300),l.kr(0.09).range(100,300),l.kr(0.06).range(10,300)],l.kr(0.09).range(1,10),10,0,0.5))
}).writeDefFile(".");

SynthDef.new(\tweet_39, { 
    Out.ar(0, GVerb.ar(Splay.arFill(32,{a=LFNoise0.kr((1..8).choose);b=[SinOsc,LFSaw].choose; b.ar(440*a,b.kr,0.1)})))
}).writeDefFile(".");

SynthDef.new(\tweet_40, { 
    Out.ar(0, GVerb.ar(IFFT(PV_BrickWall(FFT(Buffer.alloc(s,1024),WhiteNoise.ar*Pulse.ar(8,2e-2)),SinOsc.ar(Duty.kr(1,0,Dseq((10..19),inf)))))))
}).writeDefFile(".");

SynthDef.new(\tweet_41, { 
    Out.ar(0, a=SinOsc;p=Pulse;WhiteNoise.ar*p.kr(8,0.01)+a.ar(98*n=p.ar(4),0,p.kr(2,add:1))+GVerb.ar(a.ar(99*n)+p.ar(p.kr(3*n)),1,mul:0.1))
}).writeDefFile(".");

SynthDef.new(\tweet_42, { 
    Out.ar(0, p=Pulse;GVerb.ar(IFFT(PV_BrickWall(FFT(Buffer.alloc(s,128),WhiteNoise.ar*p.ar(8,5e-4)+SinOsc.ar(9*p.ar(1),0,n=p.kr(p.ar(5)))),n))))
}).writeDefFile(".");

SynthDef.new(\tweet_43, { 
    Out.ar(0, a=LFNoise0;c=PMOsc;d=Saw;e=880;c.ar(a.kr(8,e),a.kr(0.25,e)*d.kr(0.125),1,0,d.kr(8)) )
}).writeDefFile(".");

SynthDef.new(\tweet_44, { 
    Out.ar(0, p=Pulse;WhiteNoise.ar*p.ar(8,h=3e-3)+GVerb.ar(GrainSin.ar(2,p.ar(8),n=h*t=LFNoise0.ar,1/n),2)+PMOsc.ar(9*p.ar(4*t),111,p.kr(6,t,1,1)))
}).writeDefFile(".");

SynthDef.new(\tweet_45, { 
    Out.ar(0, i=Saw.kr(Saw.kr(-1/9,[3,2]).cubed);PMOsc.ar(Latch.kr(LFCub.kr(99,[0,1],99),i),Pitch.kr(i)[0][0],Decay.kr(Trig.kr(i),i*5,5)))
}).writeDefFile(".");

SynthDef.new(\tweet_46, { 
    Out.ar(0, mean({|i|99**(-1-LFSaw.kr(i+1/180,1))*SinOsc.ar(i+1*55)}!48)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_47, { 
    Out.ar(0, d=Duty.kr(Dwhite(0,LFNoise2.ar([1,1]).abs,inf),0,Dwhite(0,230,inf));GVerb.ar(BPF.ar(PinkNoise.ar(39),d.midicps,0.005).softclip,2,0.05))
}).writeDefFile(".");

SynthDef.new(\tweet_48, { 
    Out.ar(0, c=0.4;b=LFNoise1;DelayL.ar(FreeVerb.ar(a=Decay.ar(Dust.ar(c!2),c,c)*FBSineL.ar(b.ar(1,1e4,1e4),b.ar(1,8,9),1,1),1,1),1,c,c)+a)
}).writeDefFile(".");

SynthDef.new(\tweet_49, { 
    Out.ar(0, SinOsc.ar(440,0,LFSaw.kr(1,0,0.6,0.5))+SinOsc.ar(440,0,LFSaw.kr(0.2,0,0.3,0.1)))
}).writeDefFile(".");

SynthDef.new(\tweet_50, { 
    Out.ar(0, l=LFSaw;SinOsc.ar(15**(l.kr(-4.8,1)*l.kr(-1.8,1))*20).sqrt+(99**l.kr(-0.6,0.5)/99*CuspL.ar)+Blip.ar(0.8,1+LFNoise0.kr(0.2)*3e3,4)!2/4)
}).writeDefFile(".");

SynthDef.new(\tweet_51, { 
    Out.ar(0, Blip.ar(Duty.kr(1/4,0,Dshuf([60,61,63,64].midicps/8,inf))*[1,1.01]*Duty.kr(4,0,Dseq([0,-3,3,5].midiratio,inf)),6))
}).writeDefFile(".");

SynthDef.new(\tweet_52, { 
    Out.ar(0, Splay.ar({Integrator.ar(LFPulse.ar(rrand(0.1,42.0),0.3,4e-4),0.999, VarSaw.ar(LFDNoise3.kr(2.1).range(100, 3600)),0)}!22,1,0.7,0))
}).writeDefFile(".");

SynthDef.new(\tweet_53, { 
    Out.ar(0, m=MouseY.kr;i=Impulse.ar([5,5/3,1]);Env.perc(0,0.1,40*m,-8).ar(0,i[0]).cos+SinOscFB.ar(m*90,Decay.ar(i[1..],LFPar.kr(0.3)+1,9)))
}).writeDefFile(".");

SynthDef.new(\tweet_54, { 
    Out.ar(0, Splay.arFill(8,{a=(1..8).choose;b=LFNoise0.kr(a);c=LFPar.kr(a,0,b); SinOscFB.ar([63,65,67].midicps.choose,c,b)*Pulse.ar(a,c)}))
}).writeDefFile(".");

SynthDef.new(\tweet_55, { 
    Out.ar(0, x=0;(50..120).do{|f|f=f/2;x=SinOsc.ar(f+[0,1],x*Line.kr(1,3,240,doneAction:2))};tanh(x+Ringz.ar(Impulse.ar(2),45,0.3,3)))
}).writeDefFile(".");

SynthDef.new(\tweet_56, { 
    Out.ar(0, a=HPF.ar(ar(PinkNoise,5e-3),10)*Line.kr(0,1,9);ar(GVerb,({|i|ar(Ringz,a*LFNoise1.kr(0.05+0.1.rand),55*i+60,0.2)}!99).sum,70,99).tanh)
}).writeDefFile(".");

SynthDef.new(\tweet_57, { 
    Out.ar(0, LFCub.ar(LFSaw.kr(LFPulse.kr(1/4,1/4,1/4)*2+2,1,-20,50))+(WhiteNoise.ar(LFPulse.kr(4,0,LFPulse.kr(1,3/4)/4+0.05))/8)!2)
}).writeDefFile(".");

SynthDef.new(\tweet_58, { 
    Out.ar(0, LocalOut.ar(a=CombN.ar(BPF.ar(LocalIn.ar(2)*7.5+Saw.ar([32,33],0.2),2**LFNoise0.kr(4/3,4)*300,0.1).distort,2,2,40));a)
}).writeDefFile(".");

SynthDef.new(\tweet_59, { 
    Out.ar(0, a = PMOsc;b= SinOsc;c=0.004;d=440; a.ar(b.kr(0.1,d),d/2+b.kr(d*0.01,0,0.004  ),1,a.ar(4,2,1,0,a.ar(c,2,1)),b.kr(0.1)))
}).writeDefFile(".");

SynthDef.new(\tweet_60, { 
    Out.ar(0, a = PMOsc;b= SinOsc;c=RLPF;d=440;c.ar(a.ar(c.kr(b.kr(20,0,d),b.kr(0.2,0,d))),b.kr(0.01,b.kr(0.1),1).range(d,d*32)))
}).writeDefFile(".");
0.exit;
