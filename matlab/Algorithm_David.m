%% Clear everything
clear
clc

%% Setup
desiredvol = .25;
T          = 10/1000;
maxgain    = 10;
% filename   = 'lathe';
% fileext    = '.wav';
% filename   = '(50) [DJ Isaac] Burn (Sub Zero Project Remix)';
% fileext    = '.mp3';
filename   = 'generated_sine';
fileext    = '.m4a';

[input,fs] = audioread([filename,fileext]);
totallen = length(input);
blocksize = ceil((fs*T)/2)*2;
blocknum = ceil(totallen/blocksize);
input(blocksize*blocknum,1)=0; %extend input to multiple of blocks
totallen = length(input);    %length has changed, recalculate
totalwid = width(input);
output = zeros([(totallen-blocksize),totalwid]);
gain = zeros([1,(totallen-blocksize)]);
gain2 = 0; %initialize for first run

dispstat('','init'); %init status display
dispstat('Starting...','keepthis','timestamp'); 
%% Algorithm
for i = 0:(blocknum-2)
    buffer = input(((i)*blocksize+1):((i+1)*blocksize),1:totalwid);
    gain1 = gain2;
    gain2 = desiredvol/meanpower(buffer);
    if gain2 > maxgain
        gain2 = 1;
    end
    for j = 1:blocksize
        currentgain = gain1+((gain2-gain1)*j/blocksize);
        gain(((i)*blocksize)+j)=currentgain; % save for plot
        output(((i)*blocksize)+j,1:totalwid) = input((blocksize/2)+((i)*blocksize)+j,1:totalwid) * currentgain;
    end
    %statbar = statbargen((((i/(blocknum-2))*100)),20);
    dispstat(sprintf('Apply algorithm... Progress: %3.1f%%',((i/(blocknum-2))*100)),'timestamp');
end

%% Output
subplot(2,1,1)
plot((1:totallen)/fs,input)
subplot(2,1,2)
plot((1:(totallen-blocksize))/fs,output,(1:(totallen-blocksize))/fs,gain)
dispstat('Writing audio...','keepprev','timestamp');
output = output/max(abs(output));
audiowrite([filename,'_david','.m4a'],output,fs);
dispstat('Finished.','keepprev','timestamp');

%% Separate functions
function p = meanpower(Buffer)
    p1=0;
    LEN = length(Buffer);
    if width(Buffer) == 1
        for k = 1:LEN
            p1=p1+(Buffer(k))^2; 
        end 
    else
        for k = 1:LEN
            p1=p1+(Buffer(k,1))^2+(Buffer(k,2))^2; 
        end 
        p1 = p1/2;
    end
    p=sqrt(p1/LEN);
end

function w = width(input)
    [~,w] = size(input);
end

% function barstr = statbar(progress,barnum)
%     barstr = (char
% end