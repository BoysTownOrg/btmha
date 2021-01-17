% test complex FIR filterbank input/output
function tst_cffio
tst_ffio_impulse(1)
tst_ffio_tone(3)
return

%===============================================================

function tst_ffio_impulse(fig)
ifn='test/impulse.wav';
ofn='test/cffio_impulse.wav';
x=audioread(ifn);
[y,rate]=audioread(ofn);
p_lim=[-0.1 1.1];
t_lim=[-1 21];
t=1000*(0:(length(y)-1))/rate;
figure(fig);clf
plot(t,y)
title('CFIRFB impulse response')
axis([t_lim p_lim])
xlabel('time (ms)')
text(10, 0.9, sprintf('max=%.2f',max(y)))
% transfer function
figure(fig+1);clf
dc=4+1;       % desired group delay (ms)
H=ffa(y)./ffa(x);
f=linspace(0,rate/2000,length(H))';
d=dc*ones(size(f));
fm=max(f);
m_lim=[0.05 fm -5 5];
d_lim=[0.05 fm  0 8];
M=db(H);
D=gd(H,f);
subplot(2,1,1)
semilogx(f,M)
axis(m_lim)
xlabel('frequency (kHz)')
ylabel('magnitude (dB)')
title('CFIRFB transfer function')
subplot(2,1,2)
semilogx(f,D,f,d,':k')
axis(d_lim)
xlabel('frequency (kHz)')
ylabel('delay (ms)')
drawnow
%------------------------------
dev='-depsc2';
for k=1:0
   fig=sprintf('-f%d',k);
   nam=sprintf('tst_io%d.eps',k);
   print(dev,fig,nam);
end
return

function y=db(x)
y=20*log10(max(eps,abs(x)));
return

function d=gd(x,f)
p=unwrap(angle(x))/(2*pi);
d=-cdif(p)./cdif(f);
return

function dx=cdif(x)
n=length(x);
dx=zeros(size(x));
dx(1)=x(2)-x(1);
dx(2:(n-1))=(x(3:n)-x(1:(n-2)))/2;
dx(n)=x(n)-x(n-1);
return

%===============================================================

function tst_ffio_tone(fig)
ifn='test/tone.wav';
ofn='test/cffio_tone.wav';
x=audioread(ifn);
[y,rate]=audioread(ofn);
t_lim=[-0.001 0.021];
p_lim=[-1 1]*1.5;
t=(0:(length(y)-1))/rate;
figure(fig);clf
subplot(2,1,1)
plot(t,x)
axis([t_lim p_lim])
subplot(2,1,2)
plot(t,y)
axis([t_lim p_lim])
return

% ffa - fast Fourier analyze real signal
% usage: H=ffa(h)
% h - impulse response
% H - transfer function
function H=ffa(h)
H=fft(real(h));
n=length(H);
m=1+n/2;            % assume n is even
H(1,:)=real(H(1,:));
H(m,:)=real(H(m,:));
H((m+1):n,:)=[];    % remove upper frequencies
return
