<?php
/* Jongkyu Kim(j.kim@fu-berlin.de), 2016.01.12
   Adaptations by Enrico Seiler (enrico.seiler@fu-berlin.de), 2020 */
$LOCALDIR = "../";

$files = scandir($LOCALDIR, SCANDIR_SORT_DESCENDING);
$list = array();
foreach( $files as $file  )
{
    if( strpos($file, "hidden_") !== FALSE ) // Skip directories starting with "hidden_"
        continue;
    if( $file[0] == "." ) // Skip current directory
        continue;

    array_push($list, $file);
}

header("Content-Type: application/json");
echo json_encode($list, JSON_PRETTY_PRINT);
?>
