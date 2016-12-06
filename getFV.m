function [fv] = getFV(image_name)
im1 = imread(image_name);
im1gray = im2double(rgb2gray(im1));
sz = size(im1gray); 
roi_centor = int32(sz/2);
roi_sz = int32(sz/3);
roi_coord = [roi_centor(1)-roi_sz(1),roi_centor(2)-roi_sz(2)];
roi = im1gray(roi_coord(1):roi_coord(1)+roi_sz(1)-1,roi_coord(2):roi_coord(2)+roi_sz(2)-1);

hp = [ -0.0433   -0.0524   -0.0653   -0.0853   -0.1208   -0.2029   -0.6112    0.6112    0.2029    0.1208    0.0853    0.0653    0.0524    0.0433];
roi_h = roi;
for i=1:roi_sz(1)
    X = conv(roi(i,1:roi_sz(2)),hp,'same');
    roi_h(i,1:roi_sz(2)) = X;
end
roi_h = im2uint8(roi_h);

roi_v = roi;
for i=1:roi_sz(2)
    X = conv(roi(1:roi_sz(1),i),hp,'same');
    roi_v(1:roi_sz(1),i) = X;
end
roi_v = im2uint8(roi_v);

fv_h = sum((sum(roi_h))');
fv_v = sum((sum(roi_v))');
fv = [fv_h,fv_v];
% subplot(1,3,1);
% imshow(roi);
% subplot(1,3,2);
% imshow(roi_h);
% subplot(1,3,3);
% imshow(roi_v);