o = ServerOptions.new;
s = Server.remote(\pluginCollider, NetAddr("127.0.0.1", 9989), o);
s.freeAll

{ [SinOsc.ar(439, 0, 0.2), SinOsc.ar(447, 0, 0.2)] }.play(s);

{n=LFNoise0.ar(_);f=[60,61];tanh(BBandPass.ar(max(max(n.(4),l=n.(6)),SinOsc.ar(f*ceil(l*9).lag(0.1))*0.7),f,n.(1).abs/2)*700*l.lag(1))}.play(s)


~dirt = SuperDirt(2, s);
~dirt.loadSoundFiles;
~dirt.start(57120);

