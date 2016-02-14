<?php
    //!
    //! @file add_sensor_data.php
    //! @author Markus Nickels
    //! @version 0.0
    //!
    //! This script add the data sent by the ESP8266 to the database
    //!
    
    // This file is part of the Application "RoomSensor".
    //
    // RoomSensor is free software: you can redistribute it and/or modify
    // it under the terms of the GNU General Public License as published by
    // the Free Software Foundation, either version 3 of the License, or
    // (at your option) any later version.
    //
    // RoomSensor is distributed in the hope that it will be useful,
    // but WITHOUT ANY WARRANTY; without even the implied warranty of
    // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    // GNU General Public License for more details.
    //
    // You should have received a copy of the GNU General Public License
    // along with RoomSensor.  If not, see <http://www.gnu.org/licenses/>.
    
    require_once ('Smarthome.php');

	isset($_GET['locationId']) ? $locationId=$_GET['locationId'] : $locationId='1';
	isset($_GET['sensorId'])   ? $sensorId=$_GET['sensorId']     : $sensorId='1';
	isset($_GET['value'])      ? $value=$_GET['value']           : $value='25.0';

	try {

		$db = new PDO('mysql:host='.$dbHost.';dbname='.$db, $dbUser, $dbPw);

	}
	catch (PDOException $e) {
        echo "Connection failed: " . $e->getMessage();
    }

    $stmt = $db->prepare("INSERT INTO sensorData (sensorId, locationId, value) VALUES (?, ?, ?);");
    $stmt->execute(array($sensorId, $locationId, $value));
	
	echo 'done<br>';
?>
