<?php

$Ubat = $_GET['ubat'];
$Usol = $_GET['usol'];
$temp = $_GET['temp'];
$hum = $_GET['hum'];

if (!isset($Ubat)) die();
if (!isset($Usol)) die();
if (!isset($temp)) die();
if (!isset($hum)) die();

$dt = new \DateTime();
$date = $dt->format('Y-m-d H:i:s');

$fileName = 'data.txt';
$file = fopen($fileName, 'a');

if (getFileLines($fileName) <= 1)
{
  fwrite($file, "date;Ubat;Usol;temp;hum\n");  
}

fwrite($file, "$date;$Ubat;$Usol;$temp;$hum\n");  
fclose($file);
  
echo "File appended successfully";  

function getFileLines($fileName)
{
  $linecount = 0;
  $handle = fopen($fileName, "r");
  while(!feof($handle)){
    fgets($handle);
    $linecount++;
  }
  return $linecount;
}

?>