function cross_freq
fmin = 250;  % minimum crossover frequency
fmid = 1000; % mid frequency between linear and log spaced bands
nm = 5;      % number of bands below fmid
bpo = 3;     % bands per octave above fmid
sr = 24000;
nh = floor(log2(sr/2000)*bpo);
nc = nm + nh;
bw = (fmid - fmin) / (nm - 0.5);
cf=zeros(nc,1);
for i = 1:nm
    cf(i) = fmin + (i - 1) * bw;
end
for i = 1:nh
    cf(i + nm) = fmid * 2 ^ ((i - 0.5) / bpo);
end
for i=1:nc
    fprintf('%6.2f\n', cf(i) / 1000);
end
