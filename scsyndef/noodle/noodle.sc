SynthDef.new(\sineTest, {
	arg noiseHz=8, courge=10;
	var freq, amp, sig;
	freq = LFNoise0.kr(noiseHz).exprange(200,1000);
	amp = LFNoise1.kr(12).exprange(0.02,1);
	sig = SinOsc.ar(freq) * amp;
	Out.ar(0, sig);
}).writeDefFile(".");

SynthDef.new(\sc140_01, {
	Out.ar(0, a=SinOsc;l=LFNoise2;
		a.ar(666*a.ar(l.ar(l.ar(0.5))*9)*RLPF.ar(Saw.ar(9),l.ar(0.5).range(9,999),l.ar(2))).cubed)
}).writeDefFile(".");

SynthDef.new(\sc140_02, {
	Out.ar(0, a=LFSaw;Splay ar:HPF.ar(MoogFF.ar(a.ar(50*b=(0.999..9))-Blip.ar(a.ar(b)+9,b*99,9),a.ar(b/8)+1*999,a.ar(b/9)+1*2),9)/3)
}).writeDefFile(".");



0.exit;

