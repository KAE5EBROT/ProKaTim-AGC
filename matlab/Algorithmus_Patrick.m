% ======================================================
% Algorithmus AGC
% ======================================================
% 
% 14.03.2018

clear
clc

T=1;
BUFFER_LEN=T*48;  
outputpower = 50;
Buffer_in1=zeros([1,480]);
Buffer_in1(1:100)=rand([1,100])-.5;
Buffer_in1(101:200)=10*(rand([1,100])-.5);
Buffer_in1(201:480)=rand([1,280])-.5;
output = zeros([1,432]);
gain = zeros([1,432]);
g0=0;               %gain g0
g1=0;               %gain g1
sout_pegel=10;      %output mean power
BUFFER_LEN=48;   
T=1;                %T in ms von 1-100 in 1 Schritten 

% Algorithm

for i = 1:9
    buffer1 = Buffer_in1(((i-1)*BUFFER_LEN+1):((i)*BUFFER_LEN));
    buffer2 = Buffer_in1(((i)*BUFFER_LEN+1):((i+1)*BUFFER_LEN));
    gain1 = outputpower/meanpower(buffer1);
    gain2 = outputpower/meanpower(buffer2);
    for j = 1:BUFFER_LEN
        currentgain = gain1+((gain2-gain1)*j/BUFFER_LEN);
        gain(((i-1)*BUFFER_LEN)+j)=currentgain;
        output(((i-1)*BUFFER_LEN)+j) = Buffer_in1(BUFFER_LEN/2+((i-1)*BUFFER_LEN)+j) * currentgain;
    end
end

% Output
figure('Name','Input','NumberTitle','off');
plot(1:480,Buffer_in1)                     
figure('Name','Output','NumberTitle','off');
plot(1:(480-BUFFER_LEN),output,1:(480-BUFFER_LEN),gain) 


function p = meanpower(Buffer)
    p1=0;
    LEN = length(Buffer);
    for k = 1:LEN
        p1=p1+(Buffer(k))^2; 
    end 
    p=sqrt(p1/LEN);
end
