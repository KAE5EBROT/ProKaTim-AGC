clear
clc
%% Statics
N = [10 100 1000 10000 100000];

%% Dependencies
energy = zeros(size(N));
power = zeros(size(N));
amplitude = zeros(size(N));
for i = 1:length(N)
    t = (0:N(i))/N(i);

%% Calculation
    x = (2^15-1) * sin(2*pi*t);
    energy(i) = sum(x.^2);
    power(i) = energy(i)/N(i);
    amplitude(i) = sqrt(power(i));
end
%% Presentation
energy
power
amplitude

(2^15-1)/sqrt(2)

%% Conclusion
% if 0dB is the level, at which the signal maxes out the value range 
% without disturbance, the numerical value of the calculated amplitude
% equals exactly 70.7% of the maximum possible value.