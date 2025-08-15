SynthDef("testfx2b",{|out = 0, del = 0.5, vol = 1|
	var sig;
	sig = SoundIn.ar([0,1]);
	sig = sig + DelayC.ar(sig, 2, del);
	ReplaceOut.ar(out, sig*vol);
}).writeDefFile(".");

SynthDef(\simpleDelay, { arg out=0, delayTime=0.2, feedback=0.5;
    var input, delayed;
    input = In.ar(0, 1); // Assuming input from bus 0
    delayed = DelayN.ar(input, delayTime, delayTime);
    Out.ar(out, input + (delayed * feedback)); // Add delayed signal with feedback
}).writeDefFile(".");

SynthDef(\reverb, { |mix = 0.5, predelay = 0.05, decayTime = 1.0|
  var input, comb, allpass, freeverb, wet, dry;

  input = In.ar(0, 2); // Assuming stereo input

  // Predelay
  input = DelayN.ar(input, 1.0, predelay);

  // Comb filter section (can be expanded with more comb filters)
  comb = CombC.ar(input, 0.5, 0.5, decayTime);

  // Allpass filter section (can be expanded with more allpass filters)
  allpass = AllpassC.ar(comb, 0.05, 0.05, 0.2);
  allpass = AllpassC.ar(allpass, 0.03, 0.03, 0.1);

  // FreeVerb (combines comb and allpass)
  freeverb = FreeVerb.ar(allpass, mix: mix, room: decayTime, damp: 0.5);

  // Wet/Dry mix
  wet = freeverb;
  dry = input;
  Out.ar(0, wet * mix + dry * (1 - mix));
}).writeDefFile(".");

0.exit;

