var editing=false;
var xmlpath='../.../usr/rcfg/';
var xmlinfopath='../xml/';
var xmlfilename;
var xmlorg;
var xmlinfo;
var loaddefaults = false;

function loadxmlfile (xmlfilename) {
   return $.ajax({
      type: "GET",
      url: xmlpath + xmlfilename,
      dataType: "xml",
      success: function (xml1) {  
         xmlorg = xml1;
         loadxmlinfo (xmlfilename);
      },
      error: function (xhr, ajaxOptions, thrownError) {
        //alert(ajaxOptions);
        //alert('xml file not found on ' + nickname + '\ntried to access file:' + xmlpath + xmlfilename +'\nthe error was: ' + thrownError);
        if(confirm('Faild to load xml file from ' + nickname + '. Load Defaults?')) {
          loaddefaults = true;
          loadxmlinfo (xmlfilename);
        } else {
          reset();          
        }
      }             
   });
}

function loadxmlinfo (xmlfilename) {
   return $.ajax({
      type: "GET",
      url: xmlinfopath+xmlfilename+'.info',
      dataType: "xml",
      success: function (xml2) { 
         if (loaddefaults)  {
           xmlorg = xml2; 
           loaddefaults = false;
           alert('Successfully restored default values!');                   
         }
         xmlinfo = xml2;
         parsexml();
      },
      error: function (xhr, ajaxOptions, thrownError) {  
         alert('default xml file not found on ' + nickname + '\ntried to access file:' + xmlinfopath + xmlfilename + '.info' +'\nthe error was: ' + thrownError);
         reset();
      }
  });
}

function reset() {  
	$('#loader').hide();
	$('#menubuttons').hide();
   $('#response').hide();
   $('#editor').hide();
	$('#fileselector').show();
   editing=false;
}

$(function() {
	$('#loader').hide();
   $('#content').css({"visibility":"visible"});
	$('#menubuttons').hide();
   $('#response').hide();
   $('#editor').hide();
	
	// load xml files
	$('.xmlfile').unbind("mouseup keyup");
   $('.xmlfile').bind('mouseup keyup', function() { 
	   $('#fileselector').hide();  
      
	   $('#loader').show();
      xmlfilename = $(this).val();	
      if(!editing) {
         editing = true; 
         $('#response').hide();	
      	if(xmlfilename!='file') {    
	         $('#menubuttons').show(); 
            loadxmlfile(xmlfilename);               
	         $('#loader').hide(); 
	         
	         $('#resetvalues').unbind("click");
	         $('#resetvalues').bind( "click", function() {
	         	if(confirm('This will reset the values you changed to values currently stored on '+ nickname + '. Continue?')) {
                  parsexml();	
               }         	
				});
								
	         $('#defaultvalues').unbind("click");
	         $('#defaultvalues').bind( "click", function() {
	         	if(confirm('This will reset all values to default. Continue?')) {
	         		setdefault();
                  parsexml();	
               }         	
				});
         }     
      } else {
         alert('please close the open file first');      
      }      
   });   

	// save xml file
	$('#savexml').click(function() {
		
		// editing finished
      editing = false; 
      	  
	   // create a form to submit
		var formData = new FormData();	
		
      $('#response').show();
      $('#editor').hide();
      
      // read data from file and save it into xml object
      $('#response').html(getparams());		
		
		// convert xml file into blob and save the blob as file in form
   	$('#response').append( '<br />preparing xml file ...' );	
		var blob = new Blob([(new XMLSerializer()).serializeToString(xmlorg)], { type: "text/xml"});
	   formData.append('myfile',blob,xmlfilename);
		
		// send form to webserver
      var request = new XMLHttpRequest();
      request.onreadystatechange = function() {
      if (request.readyState == 4 && this.status == 200) {
         //var newwindow = window.open(xmlfilename+'.upload', 'xmlfile');
         $('#response').append(request.responseText);
         if( $('#install').length > 0 && xmlfilename != "" ) {
	            $('#install').unbind("click");
					$('#install').bind( "click", function() {
						$.ajax({
							url: "/activate?" + xmlfilename,
							success:function( htmldata ) {
								$('#response').html( htmldata );
								
	                     $('#reboot').unbind("click");
	                     $('#reboot').bind("click", function() {		
	                        if(confirm("You are about to reboot " + nickname + "'s system. Are you sure?")) {
		                     	$.ajax({ 
		                     	   url: "/reboot?",
			                        success:function( htmldata ) {
					         			   $('#response').html( htmldata );
					         			   setTimeout(function(){
					         				   document.location.href='/';
					         			   }, 30000);
					                  } 
					               });	     
	                        }
	                     });
							}
						});
					});
				}
        } else if (request.readyState == 4 && this.status != 200) {
          alert('Failed to save file\nResponse:' + request.statusText + ' ' + request.responseText);
        }
     }
      
   	$('#response').append( '<br />sending xml file ...<br />' );
      //request.open('POST', 'upload.php');
      
      // set target url and send      
      request.open('POST', '/upload.html');
      request.send(formData);
      
      // hide and show stuff
      $('#xmlparams').empty();  
      $('#editor').hide();     
	   $('#loader').hide(); 
	   $('#menubuttons').hide();
	   $('#fileselector').show();    
	});
});

function blobToFile(theBlob, fileName){
    //A Blob() is almost a File() - it's just missing the two properties below which we will add
    theBlob.lastModifiedDate = new Date();
    theBlob.name = fileName;
    return theBlob;
}

function parsexml() {
	//var xmlinfo = $.parseXML( xmlinfo )
	var item;
	var val;
	var defaultval;
   var valdesc;  
   
   $('#xmlparams').empty();  
   
   $('#xmlparams').append("<tr><th colspan=\"3\">Edit Values of \""+xmlfilename+"\"</th></tr>");
   $(xmlorg).find('parameter').each(function(){
  
      item = $(this).attr('name');
      val = $(this).attr('value');
      defaultval = $(xmlinfo).find('[name='+$(this).attr('name')+']').attr('value');
      //valdesc = $(xmlinfo).find('[name='+$(this).attr('name')+']').attr('description');
      if(!valdesc) valdesc = '';
    	
      $('#xmlparams').append('<tr><td>' + item + ':</td><td><input type="test" id="' + item + '" value="' + val + '" style="width:120px;text-align:right;" /></td><td class="defaultval">(default: ' + defaultval + ')</td></tr>');
      
   });
  
   $('loader').hide(); 
   $('#editor').show();
}

function getparams() { 
	var changeditems = '';
   $(xmlorg).find('parameter').each(function(){
   	if($(this).attr('value') != $('#'+$(this).attr('name')).val())  {
   	   $(this).attr('value',$('#'+$(this).attr('name')).val());
         changeditems = changeditems + ($(this).attr('name'))+'<br />';  	
   	}
   });
   if(changeditems.length > 0) {
      changeditems = 'The following parameters have been changed:<br />' +changeditems;
   } else {
      changeditems = 'No parameters have been changed!<br />';
   }
   
   changeditems = 'The file will now be uploaded to ' + nickname + '. After uploading you will need to <b>install the File</b> and <b>reboot</b> ' + nickname +'. The necessary buttons will be offered to you.<br />' +changeditems;
   return changeditems;
}

function setdefault() { 
   $(xmlorg).find('parameter').each(function(){
   	if($(this).attr('value') != $(xmlinfo).find('[name='+$(this).attr('name')+']').attr('value'))  {
   	   $(this).attr('value',$(xmlinfo).find('[name='+$(this).attr('name')+']').attr('value'));	
   	}
   });
}
