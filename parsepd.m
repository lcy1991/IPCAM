function [PD]=parsepd(filename)
f = fopen(filename,'r');
raw = fread(f,[4032,756],'uint16');
raw = raw';
raw = uint16(raw);
%R = raw(1:end,2:2:end);
%L = raw(1:end,1:2:end);
R = raw(1:end,1916:2:2114);
L = raw(1:end,1915:2:2113);
L_phas = zeros(1,200/2);
R_phas = zeros(1,200/2);
%use center ROI 
for i=1:100 
    L_phas(i) = sum(L(1:end,i));
    R_phas(i) = sum(R(1:end,i));
end

i = 1:1:100;
offset=(max(L_phas)+min(L_phas))/2;
L_phas_offset = L_phas - offset;
R_phas_offset = R_phas - offset;
[acor,lag] = xcorr(L_phas_offset,R_phas_offset);
[~,I]=max(acor);
R = corrcoef(L_phas_offset,R_phas_offset);
%PD = lag(I);
PD = R(1,2);
%subplot(2,1,1);
%plot(lag,acor);
% disp(X);
% subplot(4,1,1);
% imshow(L,[0,1023]);
% title('Left');

% subplot(4,1,2);
% imshow(R,[0,1023]);
% title('Right');

%subplot(2,1,2);
%plot(i,L_phas_offset,'b',i,R_phas_offset,'r');
% title('Curve');
% legend('Left','Right')
% grid on;

fclose(f);