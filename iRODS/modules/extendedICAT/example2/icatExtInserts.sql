/* 
As initialized by this file, there is no unique ID for an object.
The configured default archive in indicated by replacing 'archive' in
CADC_CONFIG_ARCHIVE_CASE with 'default'. Only one archive should be
set to default.
*/

insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','ACSIS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','2MASS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','ALMOST','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','BLAST','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','BPGS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('default','CFHT','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','CFHTSG','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','CGPS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','CGRT','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','CXO','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','DAO','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','DGO','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','DSS','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','ESOLV','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','FIRST','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','FUSE','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','GEMINI','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','GSA-SV','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','GSKY','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','HLADR2','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','HLADR3','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','HST','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','HSTCA','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','IRAS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','IRIS','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','IUE','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','JACOBY','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','JCMT','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','JCMTRR','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','JCMTSL','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','SCUPOL','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','KENNIC','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','KINNEY','U');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','MACHO','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','MOST','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','NEOSS','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','NGVS','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','ROSAT','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','STETSN','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','TULLY','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','VGPS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','WEBTMP','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','WENSS','L');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','XDSS','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','CVO','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','BLNDR','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','KVLR','N');
insert into CADC_CONFIG_ARCHIVE_CASE values ('archive','TEST','N');

insert into CADC_CONFIG_COMPRESSION values ('compression','C','CF','unknown','CFHT TEST');
insert into CADC_CONFIG_COMPRESSION values ('compression','C','Cf','unknown','CFHT TEST');
insert into CADC_CONFIG_COMPRESSION values ('compression','G','gz','gzip','BLAST CFHT CFHTSG DAO FUSE GEMINI GSA-SV GSKY HLADR2 HLADR3 HST HSTCA JCMT JCMTRR JCMTSL SCUPOL TULLY CVO CGPS VGPS TEST FIRST IRIS NEOSS STETSN MACHO DGO WEBTMP NGVS ACSIS');
insert into CADC_CONFIG_COMPRESSION values ('compression','U','Z','compress','CFHT TEST');
insert into CADC_CONFIG_COMPRESSION values ('compression','F','fz','x-fits','CFHT CFHTSG DAO HLADR2 HLADR3 TEST MACHO NGVS CGRT');

insert into CADC_CONFIG_FORMAT values ('format','D','sdf','SDF','application/octet-stream','JCMT JCMTRR SCUPOL TEST ACSIS');
insert into CADC_CONFIG_FORMAT values ('format','F','fit','FITS','application/fits','*');
insert into CADC_CONFIG_FORMAT values ('format','F','fits','FITS','application/fits','*');
insert into CADC_CONFIG_FORMAT values ('format','F','tbl','FITS','application/fits','HLADR2 HLADR3 HST DSS');
insert into CADC_CONFIG_FORMAT values ('format','G','gsd','GSD','application/octet-stream','JCMT JCMTRR TEST ACSIS');
insert into CADC_CONFIG_FORMAT values ('format','J','jpg','JPEG','image/jpeg','GEMINI CFHT DGO GSA-SV TEST HLADR2 HLADR3');
insert into CADC_CONFIG_FORMAT values ('format','J','JPG','JPEG','image/jpeg','GEMINI CFHT DGO TEST HLADR2 HLADR3');
insert into CADC_CONFIG_FORMAT values ('format','X','xml','XML','text/xml','GEMINI CFHT GSA-SV TEST WEBTMP');
insert into CADC_CONFIG_FORMAT values ('format','P','ps','PS','application/postscript','CFHT CFHTSG DGO TEST NGVS');
insert into CADC_CONFIG_FORMAT values ('format','T','txt','TXT','text/plain','BLAST DGO GEMINI CFHT JCMT JCMTRR JCMTSL TEST WEBTMP NGVS ACSIS');
insert into CADC_CONFIG_FORMAT values ('format','T','TXT','TXT','text/plain','BLAST DGO GEMINI CFHT JCMT JCMTRR JCMTSL TEST WEBTMP NGVS ACSIS');
insert into CADC_CONFIG_FORMAT values ('format','N','png','PNG','image/png','CFHT DGO TEST GSKY HLADR2 HLADR3 JCMT ACSIS');
insert into CADC_CONFIG_FORMAT values ('format','A','tar','TAR','application/x-tar','CFHT DGO GSA-SV JCMTSL TEST SCUPOL');
insert into CADC_CONFIG_FORMAT values ('format','I','gif','GIF','image/gif','CFHTSG DGO FUSE GSA-SV TEST HLADR2 HLADR3 NGVS SCUPOL');
insert into CADC_CONFIG_FORMAT values ('format',' ','','NONE','application/octet-stream','CFHT TEST');
insert into CADC_CONFIG_FORMAT values ('format','U','tif','TIFF','image/tiff','CFHT DGO TEST');
insert into CADC_CONFIG_FORMAT values ('format','Z','zip','ZIP','application/zip','ALMOST MOST TEST');
insert into CADC_CONFIG_FORMAT values ('format','M','mpg','MPEG','video/mpeg','GEMINI TEST');
insert into CADC_CONFIG_FORMAT values ('format','K','kml','KML','application/vnd.google-earth.kml+xml','GSKY');
insert into CADC_CONFIG_FORMAT values ('format','R','pdf','PDF','application/pdf','DAO DGO');
