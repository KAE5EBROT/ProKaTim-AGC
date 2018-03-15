% ======================================================
% Algorithmus AGC
% ======================================================
% 
% 14.03.2018

t=0;                % t=0: g0<g1 t=1:g1>g0
r=0;                %counter input buffer
j=0;                %counter output buffer
p=0;                %input mean power
g0=0;               %gain g0
g1=0;               %gain g1
sout_pegel=10;      %output mean power
BUFFER_LEN=48;   
T=1;                %T in ms von 1-100 in 1 Schritten 

Buffer_in_ping=ones(BUFFER_LEN,1); %Test
Buffer_in_pong=ones(BUFFER_LEN,1); %Test
Buffer_in_pung=ones(BUFFER_LEN,1); %Test
Buffer_out_ping=ones(BUFFER_LEN,1);%Test
Buffer_out_pong=ones(BUFFER_LEN,1); %Test
Buffer_out1=ones(BUFFER_LEN,1);


%% EDMA_INT
buffer= T*48;
if buffer ~= BUFFER_LEN
    r=0;                                %counter input buffer
    j=0;                                %counter output buffer
    BUFFER_LEN=buffer;
    Buffer_in_ping=ones(BUFFER_LEN,1); %Test
    Buffer_in_pong=ones(BUFFER_LEN,1); %Test
    Buffer_in_pung=ones(BUFFER_LEN,1); %Test
    Buffer_out_ping=ones(BUFFER_LEN,1);%Test
    Buffer_out_pong=ones(BUFFER_LEN,1); %Test
end

p = meanpower(Buffer_in_ping,BUFFER_LEN);

if t==0
    g0 = gain(p,sout_pegel);
    t=t+1;
else
    g1 = gain(p,sout_pegel);
    t=0;
end;

if r==0 
   Buffer_in1=Buffer_in_ping;
   Buffer_in2=Buffer_in_pong;
   r=r+1;
elseif r==1
   Buffer_in1=Buffer_in_pong;
   Buffer_in2=Buffer_in_pung;
   r=r+1; 
elseif r==2
   Buffer_in1=Buffer_in_pung;
   Buffer_in2=Buffer_in_ping;
   r=0; 
end

if j==0 
   Buffer_out1=Buffer_out_ping;
   j=j+1;
else
   Buffer_out1=Buffer_out_pong;
   j=0; 
end

Buffer_out1 = lineargain(g0,g1,t,Buffer_in1,Buffer_in2,BUFFER_LEN);



%%

function p = meanpower(Buffer,LEN)
    p1=0;
        for k = 1:LEN
            p1=p1+(Buffer(k,1))^2; 
        end 
    p=p1/LEN;
end

function g = gain(p,sout_pegel)
    g= sout_pegel/p;
end

function Buffer_out = lineargain(g0,g1,t,Buffer_in1,Buffer_in2,LEN)

i=1;

if t==0
    v= (g1-g0)/LEN;
    for k = LEN/2+1:LEN
        Buffer_out(i,1)= Buffer_in1(k,1)*(g0+v*(i-1));
        i=i+1;
    end 
    for k = 1:LEN/2
        Buffer_out(i,1)= Buffer_in2(k,1)*(g0+v*(i-1));
        i=i+1;
    end 
else
    v= (g0-g1)/LEN;
    for k = LEN/2+1:LEN
        Buffer_out(i,1)= Buffer_in1(k,1)*(g1+v*(i-1));
        i=i+1;
    end 
    for k = 1:LEN/2
        Buffer_out(i,1)= Buffer_in2(k,1)*(g1+v*(i-1));
        i=i+1;
    end 
end
end