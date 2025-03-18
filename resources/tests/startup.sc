// remote PluginCollider
o = ServerOptions.new;
s = Server.remote(\pluginCollider, NetAddr("127.0.0.1", 8898), o);

// Test sinwaves
{ [SinOsc.ar(439, 0, 0.2), SinOsc.ar(444, 0, 0.2)] }.play(s);