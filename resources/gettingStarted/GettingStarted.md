# PluginCollider Guide

Technical documentation for installing and using the **[PluginCollider](https://github.com/asb2m10/plugincollider)** VST3.

[PluginCollider](https://github.com/asb2m10/plugincollider) is a VST3 plugin that hosts SuperCollider [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html)s. This guide covers technical setup and advanced implementation for DAW integration.

---

## 1. Installation

Place the `PluginCollider.vst3` file into your system's default VST3 directory:

### 1.1. Linux
`~/.vst3/`

(or a custom folder configured in your DAW)

### 1.2. macOS
`/Library/Audio/Plug‑Ins/VST3/`

(or a custom folder configured in your DAW)

> [!IMPORTANT]
> **macOS Authorization**
> On macOS, you must authorize the plugin via Terminal after installation to bypass Gatekeeper:
> ```bash
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug‑Ins/VST3/PluginCollider.vst3
> ```
> This command modifies macOS security attributes to bypass Gatekeeper restrictions and requires administrator privileges. (Execute this command only for software you trust.)

### 1.3. Windows
`C:\Program Files\Common Files\VST3`

(or a custom folder configured in your DAW)

---

## 2. Design and Test [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html)s

- Write your instrument or effect as a [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html) in SuperCollider.
- Test it interactively in `sclang` to verify sound quality, parameter ranges, and general behavior.
- Decide which arguments you want to control from the DAW (e.g., `amp, `freq`, `attackTime`) and which ones to keep fixed internally.

- For effects processing in [PluginCollider](https://github.com/asb2m10/plugincollider), [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html)s use [SoundIn](https://docs.supercollider.online/Classes/SoundIn.html) to receive audio directly from the DAW's FX input. No [Bus](https://docs.supercollider.online/Classes/Bus.html) setup within the SuperCollider server is required. However, add `In.ar(0, 2)` to `SoundIn.ar([0, 1])` to accommodate the case of using an FX [Synth](https://docs.supercollider.online/Classes/Synth.html) together with a [MIDI](https://docs.supercollider.online/Guides/MIDI.html) [Synth](https://docs.supercollider.online/Classes/Synth.html) within a single instance of [PluginCollider](https://github.com/asb2m10/plugincollider).

---

## 3. Export [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html)s

- Use [SynthDef.writeDefFile](https://docs.supercollider.online/Classes/SynthDef.html#-writeDefFile) to export the definition into a directory that [PluginCollider](https://github.com/asb2m10/plugincollider) scans for `.scsyndef` files.

> [!NOTE]
> While you can use SuperCollider's default synthdef folder at
> `(Platform.userAppSupportDir +/+ "synthdefs")`, this location contains all `.scsyndef` files ever created by sclang.
For better organization with [PluginCollider](https://github.com/asb2m10/plugincollider), we recommend using a dedicated folder.

---

## 4. Load in DAW or Sound Editor

- Open your DAW or sound editor and insert [PluginCollider](https://github.com/asb2m10/plugincollider) as an instrument on a track (in a DAW) or as an effect from the processing menu (in a sound editor or a DAW).

> [!NOTE]
> When using [PluginCollider](https://github.com/asb2m10/plugincollider) for the first time, set the directory containing your `.scsyndef` files.

- Within [PluginCollider](https://github.com/asb2m10/plugincollider)'s interface, select the exported [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html) you want to use.

- Save this as part of your DAW project or as a plug‑in preset for use in other projects.

---

## 5. Integrate into Your Workflow

- Use MIDI tracks in your DAW to trigger SuperCollider instruments hosted by [PluginCollider](https://github.com/asb2m10/plugincollider).
- To apply audio effects, insert PluginCollider after the instrument on a MIDI track that supports virtual instruments, or place it on an audio track.
- Automate or control exposed parameters to create dynamic changes over time.
- Apply [PluginCollider](https://github.com/asb2m10/plugincollider)‑based effects to audio tracks for additional DSP processing alongside other plugins, and bounce or freeze tracks as needed.

> [!NOTE]
> - Latency may vary depending on your DAW and routing configuration.
> - When exporting [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html)s, keep file organisation clear to prevent confusion between multiple projects.

---

## 6. Examples

The following examples demonstrate how to design custom synthesis and effects in SuperCollider and integrate them seamlessly into DAW projects.

### 6.1. Using a SynthDef as a VSTi

#### Step 1: Design SynthDef and Test in SuperCollider

The following code demonstrates a complex [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html) featuring vibrato envelopes and multiple synthesis engines (Sine, Saw, Triangle, Pulse, Additive):

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

        var vibDelaySelected = Select.kr(
            vibDelaySelector.poll(0, \vibDelaySelector),
            [
                DC.kr(0).(0, \noVibDelay),
                delayStartingFromAttack.poll(0, \delayStartingFromAttack),
                vibStartingAfterTransient.poll(0, \vibStartingAfterTransient)
            ]
        ).poll(0, \vibDelaySelected);
        var vibDelayAdapted = vibDelaySelected * rand * amp.max(0).linlin(1, 0, 1, 1.5);

        var vibEnvTimes = [vibDelayAdapted, vibAccelTime, releaseTime / 2.5] * rand;
        var vibSpeedEnv = Env([0, vibStartFreq, vibPeakFreq, vibEndFreq] * rand, vibEnvTimes, \cubed, 2);
        var vibDepthEnv = Env([0, 0, 1, 0],    vibEnvTimes, \cubed, 2);
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
                    DynKlang.ar(
                        `[
                            frequencies,
                            amplitudesModifiedByFreqs,
                            \phases.kr(0 ! numHarmonics)
                    ])
                };
                [synth.(1), synth.(beatByPitchDetune.midiratio * rand)]
            }
        );

        Poll.kr(
            warnTrig,
            0,
            "WARN: vibDelayFromAttack & vibDelayPostTransient both non-zero.\n" ++
            " No sound is poduced. Set one to"
        );

        sig = sig * ampModulated * env * (1 - conflictDetector);
        sig = LeakDC.ar(sig);

        Out.ar(out, Balance2.ar(sig[0], sig[1] * phaseR, pan, amp));
    }).add
}
)
```
#### Step 2: Test in SuperCollider
```supercollider
(
Pbind(
    // \instrument, \sine,               \amp, 0.1,
    // \instrument, \triangle,           \amp, 0.05,
    // \instrument, \saw,                \amp, 0.05,
    // \instrument, \pulse, \width, 0.7, \amp, 0.05,
    \instrument, \additive, \phases, [0!16], \amplitudes, [16.collect ((_ + 1).pow(-0.8) * (_ * 0.15).neg.exp)], \amp, 0.1,
    // \out, 0,
    \freq, Pseq([69, 72, 71, 67, Prand([64, 76])].midicps, inf),
    \pan, 0,        \phaseR, -1,
    // \gate, 1,
    \attackTime, 0.03,     \decayTime, 0.3,     \sustainLevel, 0.25,   \releaseTime, 0.8,
    \beatByPitchDetune, 1/20, \randomPercent, 3,
    \vibDelayFromAttack, 0.2, \vibDelayPostTransient, 0,
    \vibStartFreq, 3,   \vibPeakFreq, 7,   \vibEndFreq, 4,     \vibAccelTime, 0.4,
    \vibAmp, 0.05,   \vibPitch, 0.05
).play;
)
```
The following code demonstrates a complex [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html) featuring vibrato envelopes and multiple synthesis engines (Sine, Saw, Triangle, Pulse, Additive) specifically designed for use within PluginCollider.

#### Step 3: Exporting for PluginCollider
After testing, export the [SynthDef](https://docs.supercollider.online/Classes/SynthDef.html) to a directory that [PluginCollider](https://github.com/asb2m10/plugincollider) scans. Adjust the path to match your own PluginCollider SynthDef folder.

> [!IMPORTANT]
>In the code snippet below, each `poll(0, …)` call has been commented out. These calls are intended solely for debugging in SuperCollider.

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

        var vibDelaySelected = Select.kr(
            vibDelaySelector/*.poll(0, \vibDelaySelector)*/,
            [
                DC.kr(0)/*.(0, \noVibDelay)*/,
                delayStartingFromAttack/*.poll(0, \delayStartingFromAttack)*/,
                vibStartingAfterTransient/*.poll(0, \vibStartingAfterTransient)*/
            ]
        )/*.poll(0, \vibDelaySelected)*/;
        var vibDelayAdapted = vibDelaySelected * rand * amp.max(0).linlin(1, 0, 1, 1.5);

        var vibEnvTimes = [vibDelayAdapted, vibAccelTime, releaseTime / 2.5] * rand;
        var vibSpeedEnv = Env([0, vibStartFreq, vibPeakFreq, vibEndFreq] * rand, vibEnvTimes, \cubed, 2);
        var vibDepthEnv = Env([0, 0, 1, 0],    vibEnvTimes, \cubed, 2);
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
                    DynKlang.ar(
                        `[
                            frequencies,
                            amplitudesModifiedByFreqs,
                            \phases.kr(0 ! numHarmonics)
                    ])
                };
                [synth.(1), synth.(beatByPitchDetune.midiratio * rand)]
            }
        );

        Poll.kr(
            warnTrig,
            0,
            "WARN: vibDelayFromAttack & vibDelayPostTransient both non-zero.\n" ++
            " No sound is poduced. Set one to"
        );

        sig = sig * ampModulated * env * (1 - conflictDetector);
        sig = LeakDC.ar(sig);

        Out.ar(out, Balance2.ar(sig[0], sig[1] * phaseR, pan, amp));
    }).writeDefFile(Platform.userAppSupportDir +/+ "Help" +/+ "Guides")
}
)
```

### 6.2. Using an Effect

You can also define DSP effects as SynthDefs for use as insert or send effects in your DAW. This example shows a delay effect with dynamically varying delay time using DelayC that processes incoming host audio.

#### Step 1: Design SynthDef and Test in SuperCollider
```supercollider
(
SynthDef(\dynamicDelays, { |varyingDelayTime = 10, mix = 0, amp = 1|
    var dry, delayTime, wet, mixed, panned;
    dry = SoundIn.ar([0, 1]).sum / 2;
    delayTime = SinOsc.ar(varyingDelayTime.reciprocal).range(0.5, 10);
    wet = DelayC.ar(dry, 10, delayTime, amp);
    wet = Limiter.ar(wet, 0.9, 0.01);
    mixed = XFade2.ar(dry, wet, mix);
    panned = Pan2.ar(mixed, SinOsc.ar(varyingDelayTime), amp);
    Out.ar(0, panned);
}).add
)
```
#### Step 2: Test in SuperCollider
```supercollider
x = Synth(\dynamicDelays) // Warning: The sound could be very loud!

x.free
```
#### Step 3: Exporting for PluginCollider

> [!IMPORTANT]
> Add `In.ar(0, 2)` to `SoundIn.ar([0, 1])` to accommodate the case of using an FX [Synth](https://docs.supercollider.online/Classes/Synth.html) together with a [MIDI](https://docs.supercollider.online/Guides/MIDI.html) [Synth](https://docs.supercollider.online/Classes/Synth.html) within a single instance of [PluginCollider](https://github.com/asb2m10/plugincollider).

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
}).writeDefFile(Platform.userAppSupportDir +/+ "Help" +/+ "Guides");
)
```

### 6.3. DAW Integration (Reaper Example)
To see these examples in action within a DAW environment, you can reference the sample Reaper project:
[Download GettingStarted.RPP by clicking `Download raw file` button.](./GettingStarted.RPP)

---

PluginCollider is an open-source project. For bug reports and contributions, please visit the GitHub Repository.
