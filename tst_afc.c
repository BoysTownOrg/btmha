# tst_afc.lst - test IIR+AGC+AFC
srate       = 24000
chunk       = 32
process     = chain
chain       = [p2.0 p1.0 p0.0 p1.1 p0.1 p1.2 p2.1]
plugin0     = iirfb
plugin1     = agc
plugin2     = afc
input       = test/cat.wav
output      = test/tst_afc.wav
