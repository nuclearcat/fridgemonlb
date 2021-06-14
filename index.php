<?php
// TODO: generate unique filename for storing db and configuration based on file path
$seed_key_filename = '.htprivate_iot_cfg.txt';
$data_filename = '.htprivate_iot_db.txt';

// This ID will generate keys for device and polling
function generateRandomString($length = 20) {
	$characters = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
	$charactersLength = strlen($characters);
	$randomString = '';
	for ($i = 0; $i < $length; $i++) {
		$randomString .= $characters[rand(0, $charactersLength - 1)];
	}
	return $randomString;
}

// TBD/FIXME: Make cron-alike function on each php call, so we can notify user if monitoring hardware is off

if (!file_exists($seed_key_filename)) {
	$unique_id = generateRandomString();
	$key_device = md5("devicesalt" . $unique_id);
	$key_poller = md5("pollersalt" . $unique_id);
	echo ("<PRE>\r\n");
	echo ("<b>Save this information! It will be shown only once, on initial generation</b>\r\n\r\n");
	// And if you didnt saved, just delete file .htprivate_iot_cfg.txt
	echo ("Seed keyword: " . $unique_id . "\r\n");
	echo ("Device key: " . $key_device . "\r\n");
	echo ("Poller key: " . $key_poller . "\r\n");
	echo ("\r\n");
	if (isset($_SERVER['HTTPS']) && $_SERVER['HTTPS'] === 'on') {
		$url = "https://";
	} else {
		$url = "http://";
	}
	// Append the host(domain name, ip) to the URL.
	$url .= $_SERVER['HTTP_HOST'];
	// Append the requested resource location to the URL
	$url .= $_SERVER['REQUEST_URI'];

	echo ("// For Arduino sketch fridgemonlb.h\r\n");
	echo ("#define IOT_URL \"" . $url . "?key=" . $key_device . "\"\r\n");
	echo ("\r\n");
	if (file_put_contents($seed_key_filename, $unique_id) == false) {
		die("<b>Error writing configuration. Please check permissions of directory on webserver!</b>");
	}
	exit(0);
} else {
	$unique_id = file_get_contents($seed_key_filename);
}

$key_device = md5("devicesalt" . $unique_id);
$key_poller = md5("pollersalt" . $unique_id);

if ($_GET{'key'} === $key_device) {
	$content = file_get_contents('php://input');
	$data = json_decode($content, true);
	$fh = fopen($data_filename, 'w');
	fwrite($fh, print_r($content, true));
	fclose($fh);
	$dataresponse = array();
	if ($data{'fw'} < 9) {
		$dataresponse{'upgrade'} = 1;
	}
	header('Content-Type: application/json');
	echo json_encode($dataresponse);
	exit(0);
}

if (isset($_GET{'key'})) {
	if ($_GET{'key'} === $key_poller) {
		$content = file_get_contents($data_filename);
		header('Content-Type: application/json');
		echo ($content);
		exit(0);
	} else {
		header('Content-Type: application/json');
		$arr["auth"] = "fail";
		echo json_encode($arr);
		exit(0);
	}
}

if ($_GET{'admin'} === $unique_id) {
	exit(0);
}
?>
<!doctype html>
<html>
<head>
	<meta name="robots" content="noindex" />
	<meta name="googlebot" content="noindex" />
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width">
	<!-- Very cool CSS framework: https://purecss.io/ -->
	<link rel="stylesheet" href="https://unpkg.com/purecss@2.0.6/build/pure-min.css" integrity="sha384-Uu6IeWbM+gzNVXJcM9XV3SohHtmWE+3VGi496jvgX1jyvDTXfdK+rfZc8C1Aehk5" crossorigin="anonymous">
	<link rel="stylesheet" href="https://code.jquery.com/ui/1.12.1/themes/base/jquery-ui.css">
	<title>Management interface</title>
    <script type="text/javascript" src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
    <script type="text/javascript" src="https://code.jquery.com/ui/1.12.1/jquery-ui.min.js"></script>
</head>
<body>
<style>
.is-center {
    text-align: center;
}
</style>
<div class="is-center" id="content">
        <h2 class="content-head is-center">Management interface</h2>
		<form class="pure-form">
    		<fieldset>
        		<input type="password" id="password" placeholder="Poller key(password)" />
        		<button type="submit" id="signin" class="pure-button pure-button-primary">Sign in</button>
    		</fieldset>
		</form>
		<div id="msg"></div>
</div>
<div id="dialog">Work in progress...</div>
<script>
var password = localStorage.getItem("pass_iot");

function poll_data(data) {
		$("#content").html('<table class="pure-table" id="datatable"><thead><tr><th>Name</th><th>Value</th><th>Settings</th></tr></thead><tbody>');
		$.each( data, function( key, value ) {
			$("#datatable").append('<tr><td>'+key+'</td><td>'+value+'</td><td><button class="ui-button ui-widget ui-corner-all setalert">Set alert</button></td></tr>');
		});
		$( ".setalert" ).click(function(event) {
			$( "#dialog" ).dialog({
  				dialogClass: "no-close",
  				buttons: [
    				{
      					text: "OK",
      					click: function() {
        					$( this ).dialog( "close" );
      					}
    				}
  				]
			});
		});
}

function try_login() {
	$("#msg").html("Please wait...");
	$.get( "?key="+password, function(data){
		if (data.auth === "fail") {
			$("#msg").html("Auth fail");
			console.log("Auth fail");
		} else {
			localStorage.setItem("pass_iot", password);
			setInterval(function() {
        		//$self.fadeOut(1000);
        		//anim_loop((index + 1) % $elements.length);
        		$.get( "?key="+password, poll_data);
    		}, 5000);
    		$.get( "?key="+password, poll_data);
		}
	});
}

if (password.length > 0) {
	try_login();
}
$("#dialog").hide();

$( "#signin" ).click(function(event) {
	event.preventDefault();

	password = $("#password").val();
	//Verify if password correct and setup loop if yes
  	//alert( "Handler for .click() called." );
});
</script>
</body>
</html>
