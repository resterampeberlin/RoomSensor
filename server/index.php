<?php
    //!
    //! @file index.php
    //! @author Markus Nickels
    //! @version 0.0
    //!
    //! General declarations for server scripts
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

    error_reporting(E_ALL);
    ini_set('display_errors','stdout');

    // print_r2($_GET);
    
    // range to be displayed
    $delta = new DateInterval('PT2M');
    
    isset($_GET['from']) ?
        $dateFrom = new DateTime($_GET['from']) :
        $dateFrom = date_sub(new DateTime('now'), new DateInterval('PT12H'));
    isset($_GET['to']) ?
        $dateTo = new DateTime($_GET['to']) :
        $dateTo = new DateTime('now');
    
    // Location to be displayed
    isset($_GET['locationId']) ?
        $locationId = $_GET['locationId'] :
        $locationId = '1';
    
    echo '<form action="index.php">';
    echo '  Start:';
    echo '  <input type="datetime-local" name="from" '.
                  'placeholder="1999-12-24 06:39" '.
                  'value="'.$dateFrom->format('Y-m-d H:i:s').'">';
    echo '  Ende:';
    echo '  <input type="datetime-local" name="to" '.
                  'placeholder="1999-12-24 06:39" '.
                  'value="'.$dateTo->format('Y-m-d H:i:s').'">';
    echo '  <input type="submit" '.
                  'value="Submit">';
    echo '</form>';
    
    echo '<center>'."\n";
    
    // Display current values
    echo '<img src="showCurrentSensorData.php'.
              '?locationId='.$locationId.'"><br>'."\n";
    
    // Display history for each sensor type
    
    // connect db
    try {
        $db = new PDO('mysql:host='.$dbHost.';dbname='.$db, $dbUser, $dbPw);
        
    }
    catch (PDOException $e)
    {
        echo 'Connection failed: '.$e->getMessage();
    }
    
    // Retrieve available sensors
    $stmt = $db->prepare('SELECT sensorType.Id '.
                         'FROM sensorData '.
                         'JOIN sensor ON sensorData.sensorId=sensor.id '.
                         'JOIN sensorType ON sensorType.Id=sensorTypeId '.
                         'WHERE sensorData.locationId=? '.
                         'AND time BETWEEN ? AND ? '.
                         'GROUP BY sensorType.Id;');
    $stmt->execute(array($locationId,
                         $dateFrom->format(DateTime::W3C),
                         $dateTo->format(DateTime::W3C)));
   
    while($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        echo '<img src="showSensorData.php'.
                   '?locationId='.$locationId.
                   '&sensorTypeId='.$row['Id'].
                   '&from='.$dateFrom->format('Y-m-d H:i:s').
                   '&to='.$dateTo->format('Y-m-d H:i:s').'">'."\n";
    }
    
    echo '</center>'."\n";
?>