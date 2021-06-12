<?php
// TODO: generate unique filename for storing db and configuration based on file path

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

if (!file_exists('.htprivate_iot_cfg.txt')) {
	$unique_id = generateRandomString();
	$key_device = md5("devicesalt" . $unique_id);
	$key_poller = md5("pollersalt" . $unique_id);
	echo ("<PRE>\r\n");
	echo ("Save this information! It will be shown only once, on initial generation\r\n");
	// And if you didnt saved, just delete file .htprivate_iot_cfg.txt
	echo ("Device key: " . $key_device . "\r\n");
	echo ("Poller key: " . $key_poller . "\r\n");
	if (file_put_contents('.htprivate_iot_cfg.txt', $unique_id) == false) {
		die("Error writing configuration. Please check permissions!");
	}
	exit(0);
} else {
	$unique_id = file_get_contents('.htprivate_iot_cfg.txt');
}

$key_device = md5("devicesalt" . $unique_id);
$key_poller = md5("pollersalt" . $unique_id);

if ($_GET{'key'} === $key_device) {
	$content = file_get_contents('php://input');
	$data = json_decode($content, true);
	$fh = fopen('.htprivate_iot_db.txt', 'w');
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

if ($_GET{'key'} === $key_poller) {

}

if ($_GET{'admin'} === $unique_id) {
	exit(0);
}