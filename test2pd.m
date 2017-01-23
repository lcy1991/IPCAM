dirss = dir('E:\AF相关\PD1610\2PD_DUMP_DATA_FULLSWEEP\*.raw');
dirs = struct2cell(dirss);
dirs_sz = size(dirs);
fileList = sortnat(dirs(1,1:dirs_sz(2)));
filtNum = size(fileList);
pd = zeros(filtNum(2),1);
for i=1:filtNum(2)
    fileName = fileList(i);
    sc = strcat('E:\AF相关\PD1610\2PD_DUMP_DATA_FULLSWEEP\',fileName{1});
    pd(i,1) = parsepd(sc);
end
plot(pd);
hold on;
