dirss = dir('*.jpg');
dirs = struct2cell(dirss);
dirs_sz = size(dirs);
fileList = sortnat(dirs(1,1:dirs_sz(2)));
steptable = 0:5:900;
filtNum = size(steptable);
fv = zeros(filtNum(2),2);
for i=1:filtNum(2)
    fileName = fileList(i);
    fv(i,1:2) = getFV(fileName{1});
end
H = fv(1:filtNum(2),1);
V = fv(1:filtNum(2),2);
plot(steptable,H);
hold on;
plot(steptable,V,'r');

