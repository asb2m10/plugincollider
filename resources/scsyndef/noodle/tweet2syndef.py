import sys
import math

template = """
SynthDef.new(\\{synthName}, {{ {arguments}
    Out.ar(0, {internalDef})
}}).writeDefFile(".");
"""

example="x=[164,166];n=Spring;n.ar(LFTri.ar(x),SinOsc.ar(x.dup(5)*HenonC.ar(x).range(rrand(0.99,1.01),1)).range(-0.5,0.5),1e-3).clip2(1))"
#example="GVerb.ar({|i|RLPF.ar(Saw.ar(1e3/(i+3),Decay.ar(Impulse.ar(1/(i+1),0.5),1)),500*i+1e3,0.4)}.dup(10).sum*0.2,77)"

class ScVarExtractor:
    idx = 0

    def __init__(self, originalCode):
        ScVarExtractor.idx += 1
        self.originalCode = originalCode
        self.arguments = {}
        pass

    def gen_arg(self, value) :
        if value[0] == "-" :
            value = "(" + value + ")"

        arg = "arg%03d" % len(self.arguments.keys())
        self.arguments[arg] = value
        return arg

    def rewrite(self):
        import pygments
        from pygments.lexers.supercollider import SuperColliderLexer

        output = []
        lexer = SuperColliderLexer()
        token = pygments.lex(self.originalCode, lexer)
        tokens = list(token)

        i = 0
        while i < len(tokens):
            t = tokens[i]

            if t[0] == pygments.token.Literal.Number.Integer or t[0] == pygments.token.Literal.Number.Float:
                if tokens[i-1][1] == "!" :
                    output.append(t[1])
                else :
                    if tokens[i-1][1] == "-":
                        value = "-" + t[1]
                        output.pop()
                    else :
                        value = t[1]

                    if tokens[i+1][1] == "b" :
                        value = float(value) - 0.1
                        i += 1
                    if tokens[i+1][1][0] == "e" :
                        if tokens[i+2][1][0] == "-" :
                            exp = int(tokens[i+3][1]) * -1
                            i += 2
                        else:
                            exp = int(tokens[i+1][1][1:])
                        value = int(value)
                        value = math.pow(value, exp)
                        i += 1
                    arg = self.gen_arg(str(value))
                    output.append(arg)
            else:
                output.append(t[1])
            i += 1
        self.newCode = "".join(output)

    def render(self):
        if len(self.arguments) == 0:
            code = self.originalCode
            arguments = ""
        else:
            arguments = "|"
            for k, v in self.arguments.items():
                arguments += "%s=%s," % (k, v)
            arguments = arguments[:-1] + "|"
            code = self.newCode

        return template.format(synthName="tweet_%02d" % self.idx, internalDef=code, arguments=arguments)

# extractor = ScVarExtractor(example)
# extractor.rewrite()
# code = extractor.render()
# print(code)
# sys.exit(1)

if __name__ == "__main__" :
    out = open("tweet2syndef.sc", "w")
    for i in open("../../resources/tests/tweets_collection.sc").readlines() :
        i = i.strip()
        if i == "" :
            continue
        if i == "//" :
            continue

        if not i.startswith("{") :
            continue
        if i.startswith("{var") :
            continue

        idx = i.rindex(".play(s)")
        i = i[1:idx-1]

        extractor = ScVarExtractor(i)
        #extractor.rewrite()
        code = extractor.render()

        out.write(code)
        print(code)

    out.write("exit(0);\n")
    out.close()
