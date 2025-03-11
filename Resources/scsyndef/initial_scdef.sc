    o = ServerOptions.new;
    s = Server.remote(\Plugincollider, NetAddr("127.0.0.1", 8898), o);

(
SynthDef("tutorial-args", { arg freq = 440, out = 0;
    Out.ar(out, SinOsc.ar(freq, 0, 0.2));
}).add
)

s.queryAllNodes

(
SynthDef(\tama,
         {|note = 52,
		note_slide = 0, note_slide_shape = 1, note_slide_curve = 0, amp = 1, amp_slide = 0, amp_slide_shape = 1, amp_slide_curve = 0, pan = 0, pan_slide = 0, pan_slide_shape = 1, pan_slide_curve = 0, attack = 0, decay = 0, sustain = 0, release = 1, attack_level = 1, sustain_level = 1, env_curve = 1, out_bus = 0, gate=1, tension=0.05, loss=0.9, vel=1, dur=1 |
		var signal, freq;
		var lossexp=LinLin.ar(loss,0.0,1.0,0.9,1.0);
		var env = Env([0, 1, 0.5, 1, 0], [0.01, 0.5, 0.02, 0.5]);
		var excitation = EnvGen.kr(Env.perc, gate, timeScale: 1, doneAction: 0) * PinkNoise.ar(0.4);
		note= VarLag.kr(note, note_slide, note_slide_curve, note_slide_shape);
		amp= VarLag.kr(amp, amp_slide, amp_slide_curve, amp_slide_shape);
		pan= VarLag.kr(pan, pan_slide, pan_slide_curve, pan_slide_shape);
		freq=note.midicps;
		signal = amp*MembraneCircle.ar(excitation, tension*(freq/60.midicps), lossexp);
		DetectSilence.ar(signal,doneAction:2);
		signal = signal * EnvGen.ar(Env.perc, gate, vel*0.5, 0, dur, 2);
		signal=Pan2.ar(signal, pan);
        Out.ar(out_bus,signal);
	}
).writeDefFile("/home/asb2m10/t");
)


(
SynthDef(\osctest,
          { |out| Out.ar(out, SinOsc.ar(440, 0, 0.2)) }
).writeDefFile("/home/asb2m10/scsyndef")
)