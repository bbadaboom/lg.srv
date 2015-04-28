// we need some vars

var intervalpointer; 
var scale = 5;  
var lastaction; 
var lastid;
var bbllines;
var drawcounter;
var drawing = 0;
var drawwdith;
var drawheight;
var drawscale;
var TO_RADIANS = Math.PI/180; 
   
// if we are ready, start the show
	window.onload = new Function("loadlist();");
	
// print map
	function loadmap(index) {
		lastaction = 'blk';
		lastid = index;
	   stopdrawing ();
		$('#'+index).css({"background-color":"#65BD36"});
		getfiletoprint(blkfiles[index],'canvas', scale);
	} 

// draw logged map
	function drawmap(index) {
		lastaction = 'bbl';
		lastid = index;
	   stopdrawing ();
		$('#'+index).css({"background-color":"#65BD36"});
		getfiletodraw(bblfiles[index],"canvas");
	} 

// stop drawing
	function stopdrawing () {
	   stoptimer();
		$('.logitem').css({"background-color":"#ffffff"});
	}
	
// load and print filelist
	function loadlist() {		  	  
	  $.each(blkfiles, function( index, file ){
       $('#maplist').append('<div id="'+index+'" class="logitem"><div><span>'+file.substr(13,2)+'.'+file.substr(11,2)+'.'+file.substr(7,4)+'</span><br /> '+file.substr(15,2)+':'+file.substr(17,2)+' Uhr</div><input type="image" src="../images/edit.png" alt="draw" width="25px" height="25px" onclick="javascript: drawmap('+index+')" /><input type="image" src="../images/zoom.png"  width="25px" height="25px" alt="show" onclick="javascript: loadmap('+index+')" /></div>');
		 $('#0').css({"background-color":"#65BD36"});
     });     
     
     $('#drawingspeed').bind('keyup mouseup', function() {
   	if(drawing == 1) {
        stoptimer();
        starttimer();
      }
     });   
     
     $('#showlegend').hover(function() {     	 
	   $('#legend').css({"visibility":"visible"});
     }, function() {
	   $('#legend').css({"visibility":"hidden"});
     });  
      
     $('#scale').bind('keyup mouseup', function() {     	
       scale = Number($('#scale').val());
       if(lastaction == 'blk') {
         loadmap(lastid);       
       } else {
       	if (lastaction == 'bbl') {
           drawmap(lastid);         
         }
       }
     });  
          
     scale = Number($('#scale').val());
     loadmap(0);
   }
 

/*###############################
########### print map ###########
###############################*/ 



function getfiletoprint( fname, canvname, scale )
{
  minX = 65536;
  minY = 65536;
  maxX = 0;
  maxY=0;

  tile_count = 0;
  TILE_SIZE = 10;
  TILE_AREA = 100;
  ROW_LENGTH = 100;
  GRID_WIDTH = 0;
  
// Check for the various File API support.
  if (!window.File || !window.FileReader || !window.FileList || !window.Blob)
  {
    alert('The File APIs are not fully supported in this browser.');
  }

  var xhr = new XMLHttpRequest();
  xhr.open('GET', '/.../usr/data/blackbox/'+fname, true );
  
  //xhr.open('GET', fname, true );

  // Hack to pass bytes through unprocessed.
  xhr.overrideMimeType('text/plain; charset=x-user-defined');

  xhr.onreadystatechange = function(e)
  {
    if (this.readyState == 4 && this.status == 200)
    {
      printmap(this.responseText,fname,canvname,scale);
    }
  };

  xhr.send();

  function printmap(texte,fname,canvname,scale)
  {
    var c=document.getElementById(canvname);

    ctx=c.getContext("2d");

    tile_count = texte.charCodeAt(32);
    pos = new Array(); //[tile_count];

    for(var i = 0; i < tile_count; i++)
    {
      p = 52 + i * 16;
      pp = (texte.charCodeAt(p) & 0xFF) + (texte.charCodeAt(p+1) & 0xFF) * 256;
      pos[i] = pp;

      xp = (pp % ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH);
      yp = (pp / ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH);

      if(xp < minX)
		minX = xp;
	  if(xp > maxX)
		maxX = xp;
      if(yp > maxY)
		maxY = yp;
      if(yp < minY)
		minY = yp;
    }

    c.width = (maxX-minX) * scale + (11 * scale);
    c.height = (maxY-minY) * scale + (11 * scale);

    start = 16044;
    for(var i = 0; i < tile_count; i++)
    {
      p = 52 + i * 16;
      pp = (texte.charCodeAt(p) & 0xFF) + (texte.charCodeAt(p+1) & 0xFF) * 256;
      pos[i] = pp;

      xp = (((pp % ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH)) - minX) * scale;
      yp = ( maxY - ((pp / ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH))) * scale;

      color = "#000000";
	
      ctx.lineWidth = 1;
      ctx.fillStyle= color;
      ctx.strokeStyle = color;
      ctx.stroke();
      //ctx.rect(xp, yp, 50, 50);
      idxTab = start + i * TILE_AREA;
      for(var j = 0; j < TILE_SIZE;j++)
      {
        for (var k = 0; k <TILE_SIZE; k++)
        {
          c = texte.charCodeAt(idxTab) >> 4;
          if(c != 0)
            color = "#FF0000"; // paroi
          else
            if(texte.charCodeAt(idxTab) != 0)
              color = "#BFBFBF";  //Bas              
	        else
              color = "#FFFFFF";
          ctx.lineWidth = 1;
          ctx.lineStyle = color;
          ctx.fillStyle= color;
          ctx.strokeStyle = color;
          ctx.stroke();
          ctx.fillRect(xp + k * scale,yp +scale - j * scale , scale, scale);
          idxTab++;
        }
      }
    }
    ctx.lineWidth = 0;

  }
}


/*##############################
########### Draw log ###########
##############################*/ 
 
// start the drawing timer
   function starttimer() {     
	   $('#stopbutton').css({"visibility":"visible"});
      speed = getspeed();
		intervalpointer = setInterval(calcnextimage, speed );
		drawing = 1;
   }
   
// stop the drawing timer
   function stoptimer() {     
	   clearInterval(intervalpointer);
	   $('#stopbutton').css({"visibility":"hidden"});
		drawing = 0;
   }

function getfiletodraw( fname, canvname ) {  
  robotImage = new Image();
  robotImage.src = "data:image/png;base64,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";
 // robotImage.onload = function() {  }
  

// Check for the various File API support.
  if (!window.File || !window.FileReader || !window.FileList || !window.Blob)
  {
    alert('The File APIs are not fully supported in this browser.');
  }

  var xhr = new XMLHttpRequest();
  xhr.open('GET', '/.../usr/data/blackbox/'+fname, true );

  //xhr.open('GET', fname, true );
  
  // Hack to pass bytes through unprocessed.
  xhr.overrideMimeType('text/plain; charset=x-user-defined');

  xhr.onreadystatechange = function(e)
  {
    if (this.readyState == 4 && this.status == 200)
    {
      drawingsetup(this.responseText,fname,canvname);
    }
  };
  xhr.send();
}

// Prepare for drawing log
function drawingsetup(bbldata,fname,canvname)
  {  
    // set the scale for next drawing
    drawscale = scale/5;
    
    //setup canvas     
    var c=document.getElementById(canvname);
    ctx = c.getContext("2d");   
         
			// Parse the lines in our bbl file into an array, one index per position
			bbllines = bbldata.split('\n');
			
			// Determine what area we need to draw
         minX = 65536;
         minY = 65536;
			maxX = 0;
			maxY = 0;							
		
			
			for (var i=0; i < bbllines.length; i++) {
					var ligne = bbllines[i].split(',');
					if (ligne[2] == 'RobotPose' ) {
						
						// Current Position
						currentX = parseInt(ligne[3].trim());
						currentY = parseInt(ligne[4].trim());
						
						// If we left the area increase the width
						if (currentX > maxX)
							maxX = currentX;
						else if (currentX < minX)
							minX = currentX;
							
					   // If we left the area increase the height
						if (currentY > maxY)
							maxY = currentY;
						else if (currentY < minY)
							minY = currentY;
					}
			}

		
		// Now we have the needed size, set size and add a buffer
		c.width = ((maxX-minX) * drawscale) / 10 + (10 +drawscale);
		c.height = ((maxY-minY) * drawscale) / 10 + (10 +drawscale);
		
      // Clear the area
		ctx.clearRect(0, 0, c.width, c.height);
		drawcounter = 0;		
		drawwidth =  robotImage.width * (drawscale);
		drawheight = robotImage.height * (drawscale);	
		starttimer();		
	}

// get drawing speed
function getspeed() {
  return (1 / $('#drawingspeed').val() * 1000);
}
   
 // We need to calc where to draw the image
 	   function calcnextimage() {
			if(drawcounter>=bbllines.length-1) { stoptimer(); }
               
               var cords = false;
               while(!cords) {               
					  var ligne = bbllines[drawcounter].split(',');
					  
					  if (ligne[2] == 'RobotPose' ||  ligne[2] == 'Bumping' || ligne[2] == 'EventPose' || ligne[2] == 'TrapStateStarts' || ligne[2] == 'TrapStateEnds' || ligne[2] == 'CoveredMap') {

						  // draw point
					     currentX = ( parseInt(ligne[3].trim()) - minX ) * drawscale;
						  currentY = ( maxY - (parseInt(ligne[4].trim())) ) * drawscale;
						
						  switch (ligne[2])
					  	  {
							  case 'RobotPose':
						       color = "#000000";
								 cords=true;
								 break;								
							  case 'Bumping':
								 color ="#FF0000";
								 cords=true;
								 break;								
							  case 'EventPose':
								 color ="#0000FF";					
								 break;																
							  case 'TrapStateEnds':
								 color ="#00FFFF";					
								 break;
							  case 'TrapStateStarts':
							  	 color ="#FF00FF";					
							  break;																
							  case 'CoveredMap':
								 color ="#AAAAAA";					
							  break;																
						  }
						
						  // Calc the angle
						  var angle = parseInt(ligne[5].trim()/100) + 90;
						
						  // Draw Luigi
		              drawRotatedImage(ctx, robotImage, currentX/10, currentY/10, angle);	
		            
		              // Mark Luigi with a colored dot	
		              if($('#colorcode').is(':checked')) {
		                ctx.beginPath();
		                ctx.arc(currentX / 10, currentY/10,(4*drawscale),0,2*Math.PI);
		                ctx.fill();
		                ctx.fillStyle = color;
                    }		            
		            
		              // Print info on data
						  //$('#drawdata').html('Data: ' + (drawcounter + 1) + ' of '+ bbllines.length );

					  }
					  drawcounter++;
					}
					cords=false;

		}
        
        
// Put image in Canvas
function drawRotatedImage(context, image, x, y, angle) { 
 
	// save the current co-ordinate system 
	// before we screw with it
	context.save(); 
 
	// move to the middle of where we want to draw our image
	context.translate(x, y);
 
	// rotate around that point, converting our 
	// angle from degrees to radians 
	context.rotate(angle * TO_RADIANS);
 
	// draw it up and to the left by half the width
	// and height of the image
	context.drawImage(image, -(drawwidth/2), -(drawheight/2),drawwidth,drawheight);
 
	// and restore the co-ords to how they were when we began
	context.restore(); 
} 