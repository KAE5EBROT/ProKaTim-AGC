clear
clc
f1=200;
f2=8005;
t=(1:(48000*10))/48000;
volume = ((1:5)/5).^3;
for i=1:5
sine(((48000*(i-1)+1):(48000*i)))=sin(2*pi*f1*t(((48000*(i-1)+1):(48000*i))))*volume(i);
sine(((48000*(i+4)+1):(48000*(i+5))))=sin(2*pi*f2*t(((48000*(i+4)+1):(48000*(i+5)))))*volume(i);
end
plot(t,sine)
audiowrite('generated_sine.m4a',sine,48000);