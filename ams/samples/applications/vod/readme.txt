#################################################################
# 		ReadMe: vod service		 	 	#
#								#
#################################################################

The vod service is an Adobe-built service that lets you stream 
media to users without writing any code.
 
Adobe Media Standard Server only runs Adobe-built services, 
also called "signed" applications.

Adobe Media Professional Server and Adobe Media 
Starter Server support unsigned (user-created) applications. If 
you're using one of these server versions, you can modify the vod 
service source code to create your own applications.


=======================================================================
Deploying custom vod service
(Adobe Media Professional Server or Adobe Media Starter Server only)
=======================================================================

To deploy an unsigned version of the vod service you can either replace 
the existing service, or create a new service.

1. [New Service] Create a new folder in the {AMS-Install-Dir}/applications 
   folder. 

2. [Existing Service] To replace the default Adobe-signed vod 
   service, first back up the following files from the folder 
   {AMS-Install-Dir}/applications/vod: 
   
   * main.far
   * Application.xml
   * allowedHTMLDomains.txt
   * allowedSWFDomains.txt 

3. Copy all files from {AMS-Install-Dir}/samples/applications/vod to 
   the folder you created in step 1 or to the existing folder, 
   {AMS-Install-Dir}/applications/vod.

4. To configure the vod service, open the file 
   {AMS-Install-Dir}/applications/{Your-New-Folder}/Application.xml.
   
   Locate the following elements:
   <Streams>/;${VOD_COMMON_DIR}</Streams>
   <Streams>/;${VOD_DIR}</Streams>

   These variables hold the location of the folder that the vod application 
   searches for media files when a stream play request is received. Their 
   values are set in the ams.ini file. For RTMP delivery, Adobe Media Server 
   checks folders for media files in the order of the <Streams> elements.
   
   ${VOD_COMMON_DIR} This variable stores the location of media files 
   accessible by both RTMP and HTTP (if installed). 
   
   ${VOD_DIR} This variable stores the location of media files accessible by 
   RTMP only through the /applications/vod service only.
   
   Alternatively, the path for media files can be changed in following ways:

	A. Edit the <Streams> element to map to a directory, for example:
	   <Streams>/;C\my_stream_and_pd_dir</Streams>
	   <Streams>/;C\my_stream_only_dir</Streams>
		Note : If you are using the default installation of the 
		       AMS HTTP service, and if you modify VOD_COMMON_DIR, 
		       change the document root, set in   
		       {AMS-Install-Dir}/Apache2.2/conf/httpd.conf. 

	B. Create a new variable in {AMS-Install-Dir}/conf/ams.ini file, 
	   for example: MY_VOD_DIR= C\my_stream_dir. 
	   Next, edit the <Streams> element in the Application.xml file:
	      <Streams>/;${MY_VOD_COMMON_DIR}<Streams>
	      <Streams>/;${MY_VOD_DIR}</Streams>

-------------------------------------------------------------------------

For information about using and configuring the live service, see the 
online documentation provided at the following location:
http://help.adobe.com/en_US/adobemediaserver/devguide

For information about troubleshooting the live service, see the 
Installation Guide in the {AMS-Install-Dir}/documentation folder. 

