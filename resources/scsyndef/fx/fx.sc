SynthDef("testfx2b",{|out = 0, del = 0.5, vol = 1|
	var sig;
	sig = SoundIn.ar([0,1]);
	sig = sig + DelayC.ar(sig, 2, del);
	ReplaceOut.ar(out, sig*vol);
}).writeDefFile(".");

0.exit;

