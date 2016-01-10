function drawSnapshot()
{
     var
         yuvUrl = 'images/snapshot.yuv',
         canvas = document.getElementById('cam_canvas'),
         context = canvas.getContext('2d'),
         width = canvas.width,
         height = canvas.height,
         outputImage = context.createImageData(width, height),
         outputImageData = outputImage.data
     ;
     var xhr = new XMLHttpRequest();
     xhr.open('GET', yuvUrl, true);
     xhr.responseType = 'arraybuffer';
     xhr.onload = function() {
         var yuvData = new Uint8Array(xhr.response), y, u, v;
         for (var i = 0, p = 0; i < outputImageData.length; i += 4, p += 1)
         {
             y = yuvData[ p ];
             v = yuvData[ Math.floor(width * height * 1.5 + p /2) ];
             u = yuvData[ Math.floor(width * height + p / 2) ];
             outputImageData[i]   = y + 1.371 * (v - 128);
             outputImageData[i+1] = y - 0.336 * (u - 128) - 0.698 * (v - 128);
             outputImageData[i+2] = y + 1.732 * (u - 128);
             outputImageData[i+3] = 255;
         }
         context.putImageData(outputImage,0,0);
         window.setTimeout('drawSnapshot()', 1000);
     }
     xhr.send(null);
}
