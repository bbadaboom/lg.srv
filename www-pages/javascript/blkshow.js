function BlkShow( fname, canvname )
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

  // Hack to pass bytes through unprocessed.
  xhr.overrideMimeType('text/plain; charset=x-user-defined');

  xhr.onreadystatechange = function(e)
  {
    if (this.readyState == 4 && this.status == 200)
    {
      setup(this.responseText,fname,canvname);
    }
  };

  xhr.send();

  function setup(texte,fname,canvname)
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

    c.width = (maxX-minX) * 2 + 22;
    c.height = (maxY-minY) * 2 + 22;

    start = 16044;
    for(var i = 0; i < tile_count; i++)
    {
      p = 52 + i * 16;
      pp = (texte.charCodeAt(p) & 0xFF) + (texte.charCodeAt(p+1) & 0xFF) * 256;
      pos[i] = pp;

      xp = (((pp % ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH)) - minX) * 2;
      yp = ( maxY - ((pp / ROW_LENGTH) * (TILE_SIZE + GRID_WIDTH))) * 2;

      color = "#000000";
	
      ctx.lineWidth = 1;
      ctx.fillStyle= color;
      ctx.strokeStyle = color;
      ctx.stroke();
//      ctx.rect(xp, yp, 50, 50);
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
          ctx.fillRect(xp + k * 2,yp +2 - j * 2 , 2, 2);
          idxTab++;
        }
      }
    }
    ctx.lineWidth = 0;
	ctx.font="9px Verdana";
	ctx.strokeStyle="#2020ff";
	ctx.strokeText(fname,5,190);

  }
}
