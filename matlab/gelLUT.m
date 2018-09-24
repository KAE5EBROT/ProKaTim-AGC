x = round(10.^((((0:96)-9)/20)));
lastx = 0;
for i = 1:length(x)
    if x(i) <= lastx
        x(i) = lastx +1;
    end
    lastx = x(i);
end
    
varname1 = "thresh_value";
varname2 = "minimal_power";
disp("if ("+varname1+" == 0)");
disp("    "+varname2+" = "+int2str(x(1))+";");
for i = 2:97
disp('else if ('+varname1+' == '+int2str(i-1)+')');
disp("    "+varname2+" = "+int2str(x(i))+";");
end

disp('');