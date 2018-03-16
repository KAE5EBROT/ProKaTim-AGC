%% Clear everything
clear
clc

%% Setup
desiredpower = 5;
input = zeros([1,1000]);
input(1:200)    = 1 * (rand([1,200])-.5);
input(201:400)  = 3 * (rand([1,200])-.5);
input(401:600)  = 8 * (rand([1,200])-.5);
input(601:800)  = 2 * (rand([1,200])-.5);
input(801:1000) = 5 * (rand([1,200])-.5);
input = awgn(input,5);

output = zeros([1,900]);
gain = zeros([1,900]);

%% Algorithm
for i = 1:9
    buffer1 = input(((i-1)*100+1):((i)*100));
    buffer2 = input(((i)*100+1):((i+1)*100));
    gain1 = desiredpower/meanpower(buffer1);
    gain2 = desiredpower/meanpower(buffer2);
    for j = 1:100
        currentgain = gain1+((gain2-gain1)*j/100);
        gain(((i-1)*100)+j)=currentgain; % save for plot
        output(((i-1)*100)+j) = input(50+((i-1)*100)+j) * currentgain;
    end
end

%% Output
plot(1:900,output,1:900,gain)
figure
plot(1:1000,input)

%% Separate functions
function p = meanpower(Buffer)
    p1=0;
    LEN = length(Buffer);
    for k = 1:LEN
        p1=p1+(Buffer(k))^2; 
    end 
    p=sqrt(p1/LEN);
end