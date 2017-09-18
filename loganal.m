%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Bebop drone Log analyzer
% Ploting the PCMD and Vx (dx), Vy (dy), Delat-Yaw, Altitude
% last update 2017. 09.18
% (c)  ICOMLAB, SeoulTech, South korea
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%logFileName = 'drone-2017-09_18_06_14_36.log';
logFileName = 'drone-2017-09_18_06_19_13.log';
fid = fopen(logFileName);

nfst = 1;
fst = [];
nspd = 1;
spd = [];
natt = 1;
att = [];
nalt = 1;
alt = [];
npcmd = 1;
pcmd = [];

tline = fgetl(fid);
while ischar(tline)
   % disp(tline)
    %tag = sscanf(tline,'%s ');
    tag = textscan(tline,'%s',1);
   % disp(tag)
    
    switch (tag{1}{1})
        case 'fst'
             newlog = sscanf(tline,'%s %u %d');
             fst = [fst; newlog(4:5)'];
        case 'pcmd'
             newlog = sscanf(tline,'%s %u %d %d %d %d');
             if(size(pcmd,1) > 0)
                pcmd = [pcmd; [newlog(5)-1, pcmd(end:end,2:5)]];
             end
             pcmd = [pcmd; newlog(5:9)'];
        case 'spd'
              newlog = sscanf(tline,'%s %u %f %f %f');
              spd = [spd; newlog(4:7)'];
        case  'att'
              newlog = sscanf(tline,'%s %u %f %f %f');
              att = [att; newlog(4:7)'];
        case 'alt'
             newlog = sscanf(tline,'%s %u %f');
             alt = [alt; newlog(4:5)'];
    end
    
    tline = fgetl(fid);
end

fclose(fid);

% spd to postion

% ploting
subplot(4, 1, 1);
hold on;
plot(pcmd(:,1), pcmd(:,5)/10, 'k');
plot(alt(:,1), alt(:,2), '.k');
xlabel('msec');
title('GAZ/10 or alt(m)');
grid;

xpos = zeros(1,length(spd(:,2)));
for n = 2: length(spd(:,2))
    xpos(n) = xpos(n-1) + (spd(n-1,2) + spd(n,2))/2 *(spd(n,1) - spd(n-1,1))/1000.0;
end
subplot(4,1,2);
hold on;
plot(pcmd(:,1), pcmd(:,2)/10, 'r');
plot(spd(:,1), spd(:,2), '.r');
plot(spd(:,1), xpos, '-.r');
xlabel('msec');
title('PITCH/10 or Vx(m)');
grid;

ypos = zeros(1,length(spd(:,3)));
for n = 2: length(spd(:,3))
    ypos(n) = ypos(n-1) + (spd(n-1,3) + spd(n,3))/2 *(spd(n,1) - spd(n-1,1))/1000.0;
end
subplot(4,1,3);
hold on;
plot(pcmd(:,1), pcmd(:,3)/10, 'g');
plot(spd(:,1), spd(:,3), '.g');
plot(spd(:,1), ypos, '-.g');
xlabel('msec');
title('Roll/10 or Vy(m)');
grid;

subplot(4,1,4);

att(:,4) = att(:,4) - att(1,4);
hold on;
plot(pcmd(:,1), pcmd(:,4)/10, 'b');
plot(att(:,1), att(:,4), '.b');
xlabel('msec');
title('rot/10 or delta yaw(rad)');
grid;


