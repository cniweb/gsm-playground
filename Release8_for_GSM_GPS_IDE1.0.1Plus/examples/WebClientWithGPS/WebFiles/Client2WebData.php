<?php
	// read parameters
	$ID = htmlspecialchars($_GET['id'], ENT_QUOTES, "UTF-8");
	$TEMP = htmlspecialchars($_GET['temp'], ENT_QUOTES, "UTF-8");
	$USER_BUTTON = htmlspecialchars($_GET['user_button'], ENT_QUOTES, "UTF-8");
	$GPIO10 = htmlspecialchars($_GET['GPIO10'], ENT_QUOTES, "UTF-8");
	$GPIO11 = htmlspecialchars($_GET['GPIO11'], ENT_QUOTES, "UTF-8");
	$GPS_VALID = htmlspecialchars($_GET['GPS_valid'], ENT_QUOTES, "UTF-8");
	$GPS_LATITUDE = htmlspecialchars($_GET['GPS_latitude'], ENT_QUOTES, "UTF-8");
	$GPS_LONGITUDE = htmlspecialchars($_GET['GPS_longitude'], ENT_QUOTES, "UTF-8");
	 
	// and write parameters to the file
	if ($TEMP)
	    {
	        if ($file=@fopen("Client2WebData.dat", "w"))
	        {
				$text=fwrite($file,$ID);
				$text=fwrite($file,";");
				$text=fwrite($file,Date("d.m.Y;H:i:s"));
				$text=fwrite($file,";");
	            $text=fwrite($file,$TEMP);
				$text=fwrite($file,";");
	            $text=fwrite($file,$USER_BUTTON);
				$text=fwrite($file,";");
				$text=fwrite($file,$GPIO10);
				$text=fwrite($file,";");
				$text=fwrite($file,$GPIO11);
				$text=fwrite($file,";");
	            $text=fwrite($file,$GPS_VALID);
				$text=fwrite($file,";");
	            $text=fwrite($file,$GPS_LATITUDE);
				$text=fwrite($file,";");
	            $text=fwrite($file,$GPS_LONGITUDE);
	            fclose($file);
				
				// send data back
				if ($file=@fopen("WebData2Client.dat", "r"))
				{
					echo "RET_S;OK;";	// RET_S = RET_START
					$text=fread($file,FileSize("WebData2Client.dat"));
					fclose($file);
					List($user_led, $GPIO12, $GPIO13) = Explode(";", $text);
					echo $user_led;
					echo ";";
					echo $GPIO12;
					echo ";";
					echo $GPIO13;
					echo ";";
					echo "RET_E"; // RET_E ´RET_END
				}
				else {
					echo "RET_S;ERROR;Cannot open a file WebData2Client.dat!;RET_E";
				}
				
	        }
	        else
	        {
	            echo "RET_S;ERROR;File Client2WebData.dat doesn't have rights for write!;RET_E";
	        }
	}
?>
	
