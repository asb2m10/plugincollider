// remote pluginCollider
o = ServerOptions.new;
s = Server.remote(\pluginCollider, NetAddr("127.0.0.1", 8898), o);

// Taken from https://scsynth.org/t/map-value-range-of-midi-control-bus/6588
(
var mkMappedBus = {|sym, defaultRange=([0, 1]), type=\linlin| 
    var v = NamedControl.kr(sym);
    var range = NamedControl.kr(sym ++ 'Range', defaultRange);
    v.perform(type, 0, 1, *range)
};

x = SynthDef("freqtest", 
    {   var freq = mkMappedBus.(\freq, [20, 20000], \linexp);
        var sig = SinOsc.ar(freq);
        Out.ar(sig, sig!2 * 1);
    }).play(s);
)

b = Bus.control(s, 1);
x.map(\freq, b)








