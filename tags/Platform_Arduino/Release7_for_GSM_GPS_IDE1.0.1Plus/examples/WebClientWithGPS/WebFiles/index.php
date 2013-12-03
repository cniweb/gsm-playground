<meta charset="iso-8859-2">
<H1>HW Kitchen WebClient example</h1>
<!-- this is a comment -->
<!-- <meta http-equiv='refresh' content='2' > -->
<hr>
<?php
	if ($file=@fopen("Client2WebData.dat", "r"))
	{
		$text=fread($file,FileSize("Client2WebData.dat"));
	    fclose($file);
		List($id, $date, $time, $temperature, $user_button, $GPIO10, $GPIO11, $GPS_valid, $GPS_latitude, $GPS_longitude) = Explode(";", $text);
		echo "GSM module: ";
		echo $id;
		echo "<br/>";
		
		echo "Data & Time: ";
		echo $date." ";
		echo $time." ";
		echo "<br/>";
		
		echo "GSM module temperature: ";
	    echo $temperature." C";
		echo "<br/>";
		echo "<br/>";
		
		echo "User button: ";
	    echo $user_button;
		echo "<br/>";
		
		echo "GPIO10 state: ";
	    echo $GPIO10;
		echo "<br/>";
		
		echo "GPIO11 state: ";
	    echo $GPIO11;
		echo "<br/>";
		echo "<br/>";

		
		if ($GPS_valid) {	
			echo "GPS_latitude: ";
	    	echo $GPS_latitude;
			echo "<br/>";
			echo "GPS_longitude: ";
	    	echo $GPS_longitude;
			echo "<br/>";
		}
	
		
		//echo "<hr>";

			
	}
	else
	{
	    echo "ERROR - file Client2WebData.dat doesn't have rights for reading!";
	}
?>

		
<?php
if ($GPS_valid) {
	//example of string - we need: "https://maps.google.com/maps?q=50.003995,18.08916";
	/*this is an example of HTML code generates calling of google maps
		<a href="https://maps.google.com/maps?q=50.003995,18.08916" target=_blank>Click here to show actual location</a>
	*/

	$retez = "https://maps.google.com/maps?q=".$GPS_latitude.$GPS_longitude;
?>
	<a href="<?php echo $retez?>" target=_blank>Click here to show actual location</a>
<?php
}
?>

<hr>
<form method="GET" action="WebData2Client.php">

<!--	
<p>User LED state:<BR>
<input type="text" name="LED_state" size="16"> 
   <input type=submit value=Send>
	
<p>LED state through 2 buttons:	
<INPUT TYPE=SUBMIT NAME="LED_state_1" VALUE="Activate">
<INPUT TYPE=SUBMIT NAME="LED_state_1" VALUE="Deactivate">
-->
	
<!-- Read last state of outputs. Will be used for initialization of Radio buttons -->	
<?	
if ($file=@fopen("WebData2Client.dat", "r"))
{
	$text=fread($file,FileSize("WebData2Client.dat"));
	fclose($file);
	List($user_LED_state, $GPIO12_state, $GPIO13_state) = Explode(";", $text);
}
?>
	
	

<!-- Radio button User LED -->
<p>User LED:<BR>
<?
if ($user_LED_state == 0) {
?>
	<INPUT TYPE=RADIO NAME=user_LED VALUE=0 checked="checked">Deactivate<BR>
	<INPUT TYPE=RADIO NAME=user_LED VALUE=1>Activate<BR>
<?php
}	
else {
?>	
	<INPUT TYPE=RADIO NAME=user_LED VALUE=0>Deactivate<BR>
	<INPUT TYPE=RADIO NAME=user_LED VALUE=1 checked="checked">Activate<BR>
<?php
}
?>
	
<!-- Radio button GPIO12 -->
<p>GPIO12:<BR>
<?
if ($GPIO12_state == 0) {
?>
	<INPUT TYPE=RADIO NAME=GPIO12 VALUE=0 checked="checked">Deactivate<BR>
	<INPUT TYPE=RADIO NAME=GPIO12 VALUE=1>Activate<BR>
<?php
}	
else {
?>	
	<INPUT TYPE=RADIO NAME=GPIO12 VALUE=0>Deactivate<BR>
	<INPUT TYPE=RADIO NAME=GPIO12 VALUE=1 checked="checked">Activate<BR>
<?php
}
?>	
	
<!-- Radio button GPIO13 -->
<p>GPIO13:<BR>
<?
if ($GPIO13_state == 0) {
?>
	<INPUT TYPE=RADIO NAME=GPIO13 VALUE=0 checked="checked">Deactivate<BR>
	<INPUT TYPE=RADIO NAME=GPIO13 VALUE=1>Activate<BR>
<?php
}	
else {
?>	
	<INPUT TYPE=RADIO NAME=GPIO13 VALUE=0>Deactivate<BR>
	<INPUT TYPE=RADIO NAME=GPIO13 VALUE=1 checked="checked">Activate<BR>
<?php
}
?>		

	
<BR>	
<input type=submit value=Update_outputs>	
	

	
	
	
</form>
	
<html>
<br/>
<br/>
<br/>
<hr>
<endora>
</html>