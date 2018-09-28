clear
clc
fs = 48000;
f1=5397;
f2=379;
t=(1:(fs*10))/fs;
volume = ((1:5)/5).^3;
sine = zeros(size(t));
for i=1:5
sine(((fs*(i-1)+1):(fs*i)))=sin(2*pi*f1*t(((fs*(i-1)+1):(fs*i))))*volume(i) .* (.2*sin(2*pi*493*t((fs*(i-1)+1):(fs*i)))+.8);
sine(((fs*(i+4)+1):(fs*(i+5))))=sin(2*pi*f2*t(((fs*(i+4)+1):(fs*(i+5)))))*volume(i);
end
plot(t,sine)
audiowrite('generated_sine.m4a',sine,fs);