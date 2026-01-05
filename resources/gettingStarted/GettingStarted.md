# PluginCollider Guide

Technical documentation for installing and using the **PluginCollider** VST3.

[PluginCollider](https://github.com/asb2m10/plugincollider) is a VST3 plugin that hosts SuperCollider SynthDefs. This guide covers technical setup and advanced implementation for DAW integration.

---

## Installation

Place the `PluginCollider.vst3` file into your system's default VST3 directory:

### Linux
`~/.vst3/`

### macOS
`/Library/Audio/Plug‑Ins/VST3/`

> [!IMPORTANT]
> **macOS Authorization**
> On macOS, you must authorize the plugin via Terminal after installation to bypass Gatekeeper:
> ```bash
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug‑Ins/VST3/PluginCollider.vst3
> ```

### Windows
`C:\Program Files\Common Files\VST3`

---

## Instrument Example

The following code demonstrates a complex `SynthDef` featuring vibrato envelopes and multiple synthesis engines (Sine, Saw, Triangle, Pulse, Additive) specifically designed for use within PluginCollider.

```supercollider
(
[\sine, \saw, \triangle, \pulse, \additive].do { |name|
	SynthDef(name, { |out = 0, freq = 440, amp = 0.1, pan = 0, phaseR = 1, gate = 1,
		attackTime = 0.07, decayTime = 0.08, sustainLevel = 0.1, releaseTime = 0.3,
		beatByPitchDetune = 0.041666666666667,
		vibDelayFromAttack = 0.2, vibDelayPostTransient = 0,
		vibStartFreq = 3, vibPeakFreq = 7, vibEndFreq = 4, vibAccelTime = 0.4,
		vibAmp = 0.02, vibPitch = 0.0625, randomPercent = 6.25|

		var rand = 2 ** (Rand(randomPercent / -2, randomPercent / 2) / 1200);
		var conflictDetector = (vibDelayFromAttack > 0) * (vibDelayPostTransient > 0);
		var warnTrig = Trig1.kr(conflictDetector, 0.001);
		var transientAtBeginning = attackTime + (decayTime * 1.1);
		var vibStartingAfterTransient = transientAtBeginning + vibDelayPostTransient;
		var delayStartingFromAttack = vibDelayFromAttack;
		var selVibDelayFromAttack = (vibDelayFromAttack > 0) * (vibDelayPostTransient |==| 0);
		var selVibDelayPostTransient = (vibDelayFromAttack |==| 0) * (vibDelayPostTransient > 0);
		var vibDelaySelector = (selVibDelayFromAttack * 1) + (selVibDelayPostTransient * 2);

		var vibDelaySelected = Select.kr(vibDelaySelector, [
			DC.kr(0),
			delayStartingFromAttack,
			vibStartingAfterTransient
		]);
		var vibDelayAdapted = vibDelaySelected * rand * amp.max(0).linlin(1, 0, 1, 1.5);

		var vibEnvTimes = [vibDelayAdapted, vibAccelTime, releaseTime / 2.5] * rand;
		var vibSpeedEnv = Env([0, vibStartFreq, vibPeakFreq, vibEndFreq] * rand, vibEnvTimes, \cubed, 2);
		var vibDepthEnv = Env([0, 0, 1, 0],	vibEnvTimes, \cubed, 2);
		var vibDepthEnvGen = EnvGen.kr(vibDepthEnv, gate);

		var lfo = (SinOsc.kr(EnvGen.kr(vibSpeedEnv, gate), -pi/2, 1.557407724655)).atan;
		var vibPitchDepth = vibPitch * vibDepthEnvGen * rand;
		var modulatorPitch = lfo.linlin(-1, 1, 0, 1) * vibPitchDepth;
		var freqModulated = freq * modulatorPitch.midiratio;

		var vibAmpDepth = (vibAmp * vibDepthEnvGen).clip(0, 1);
		var modulatorAmp = lfo.linlin(-1, 1, 0, vibAmpDepth);
		var ampModulated = (1 - modulatorAmp).clip(0, 1);

		var env = Env.adsr(attackTime * rand, decayTime * rand, sustainLevel * rand,
			releaseTime, curve: -4).kr(Done.freeSelf, gate);

		var sig = switch(name,
			\sine, { SinOsc.ar(freqModulated * [1, beatByPitchDetune.midiratio * rand]) },
			\saw, { Saw.ar(freqModulated * [1, beatByPitchDetune.midiratio * rand]) },
			\triangle, { LFTri.ar(freqModulated * [1, beatByPitchDetune.midiratio * rand]) },
			\pulse, { Pulse.ar(freqModulated * [1, beatByPitchDetune.midiratio * rand], \width.kr(0.5)) },
			\additive, {
				var numHarmonics = 16;
				var synth = { |channelFreqs|
					var frequencies = freqModulated * channelFreqs *.t (1..numHarmonics);
					var amplitudes = \amplitudes.kr(1 ! numHarmonics / numHarmonics);
					var amplitudesModifiedByFreqs = frequencies.size.collect { |i|
						Select.kr(frequencies[i] > (SampleRate.ir / 2), [amplitudes[i], 0])
					};
					DynKlang.ar(`[frequencies, amplitudesModifiedByFreqs, \phases.kr(0 ! numHarmonics)])
				};
				[synth.(1), synth.(beatByPitchDetune.midiratio * rand)]
			}
		);

		sig = sig * ampModulated * env * (1 - conflictDetector);
		sig = LeakDC.ar(sig);
		Out.ar(out, Balance2.ar(sig[0], sig[1] * phaseR, pan, amp));
	}).writeDefFile(Platform.userAppSupportDir +/+ "PluginColliderSynthDefs");
}
)
```

---

## Effects Processing
For DSP effects, use SoundIn to receive audio directly from the DAW's track or FX bus.

```supercollider
(
SynthDef(\dynamicDelays, { |varyingDelayTime = 10, mix = 0, amp = 1|
	var dry, delayTime, wet, mixed, panned;
	dry = (In.ar(0, 2) + SoundIn.ar([0, 1])).sum / 2;
	delayTime = SinOsc.ar(varyingDelayTime.reciprocal).range(0.5, 10);
	wet = DelayC.ar(dry, 10, delayTime, amp);
	wet = Limiter.ar(wet, 0.9, 0.01);
	mixed = XFade2.ar(dry, wet, mix);
	panned = Pan2.ar(mixed, SinOsc.ar(varyingDelayTime), amp);
	Out.ar(0, panned);
}).writeDefFile(Platform.userAppSupportDir +/+ "PluginColliderSynthDefs");
)
```

## DAW Integration (Reaper Example)
To see these examples in action within a DAW environment, you can reference the sample Reaper project:
[Download GettingStarted.zip](./GettingStarted.zip)

[!NOTE] The .RPP file must be located in your SuperCollider Help/Guides directory for the following command to work.



---

PluginCollider is an open-source project. For bug reports and contributions, please visit the GitHub Repository.
