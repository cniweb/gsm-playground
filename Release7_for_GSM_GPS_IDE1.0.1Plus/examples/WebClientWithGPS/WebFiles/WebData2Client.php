<meta charset="iso-8859-2">
<?php
	$USER_LED = htmlspecialchars($_GET['user_LED'], ENT_QUOTES, "UTF-8");
	$GPIO12 = htmlspecialchars($_GET['GPIO12'], ENT_QUOTES, "UTF-8");
	$GPIO13 = htmlspecialchars($_GET['GPIO13'], ENT_QUOTES, "UTF-8");
	 
	// and write parameters to the file
	if ($file=@fopen("WebData2Client.dat", "w"))
	{
		$text=fwrite($file,$USER_LED);
		$text=fwrite($file,";");
		$text=fwrite($file,$GPIO12);
		$text=fwrite($file,";");
		$text=fwrite($file,$GPIO13);
		$text=fwrite($file,";");
		fclose($file);
		echo "OK";
	}
	else
	{
		echo "ERROR - file WebData2Client.dat doesn't have rights for write!";
	}
?>
<meta http-equiv="refresh" content="1;url=index.php">