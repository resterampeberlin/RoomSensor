<?php
    //!
    //! @file show_current_sensor_data.php
    //! @author Markus Nickels
    //! @version 0.0
    //!
    //! This script shows the latest sensor data as a diagram
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

    require_once ('../jpgraph/src/jpgraph.php');
    require_once ('../jpgraph/src/jpgraph_bar.php');
    require_once ('../jpgraph/src/jpgraph_error.php');
    
    //
    // Main
    //
    
    $xAxis = array();
    $yAxis = array();
    
    // Basic constants for display
    //print_r2($_GET);
    
    // Location to be displayed
    isset($_GET['locationId']) ?
            $locationId=$_GET['locationId'] :
            $locationId='1';
    
    // connect db
	try {
		$db = new PDO('mysql:host='.$dbHost.';dbname='.$db, $dbUser, $dbPw);
    }
	catch (PDOException $e)
    	{
    		echo "Connection failed: " . $e->getMessage();
    	}
    
    
    // Get location name
    $stmt = $db->prepare('SELECT * '.
                         'FROM location '.
                         'WHERE id=?;');
    $stmt->execute(array($locationId));
    $row = $stmt->fetch(PDO::FETCH_ASSOC);
    $locationName = $row['name'];
    
    // Retrieve last available sensor data
    $stmt = $db->prepare('SELECT sensorId,'.
                            'MAX(time) AS time,'.
                            'value,'.
                            'sensor.name AS name,'.
                            'sensorType.name AS class,'.
                            'sensorType.unit '.
                         'FROM sensorData '.
                         'JOIN sensor ON sensor.id=sensorData.sensorId '.
                         'JOIN sensorType ON sensorType.id=sensor.sensorTypeId '.
                         'WHERE locationId=? '.
                         'GROUP BY sensorId DESC');
    $stmt->execute(array($locationId));
    
    if ($stmt->rowCount() == 0) {
        echo 'No data found for '.$locationName.'<br>';
        exit(1);
    }
    
    $i = 0;
    while($stmt && $row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $xAxis[$i] = $row['name'];
        $yAxis[$i] = $row['value'];

        $lastTime = new DateTime($row['time']);
        $unit = $row['class'].'['.$row['unit'].']';
        $i++;
    }
    
    //print_r2($xAxis);
    //print_r2($yAxis);
    
    $graph = new Graph(1200,280);
    $graph->img->SetMargin(40,40,40,40);
    $graph->img->SetAntiAliasing();
    $graph->SetScale('textlin');
    $graph->SetShadow();
    
    // Title
    $graph->title->Set('Letzte Sensorwerte '.$locationName);
    $graph->title->SetFont(FF_ARIAL, FS_BOLD, 12);
    $graph->subtitle->SetFont(FF_ARIAL, FS_NORMAL, 10);
    $graph->subtitle->Set($lastTime->format('d.m.y H:i:s'));

    // X-Axis
    $graph->xaxis->SetTickLabels($xAxis);
    $graph->xaxis->SetTitle('Sensor','middle');
    $graph->xaxis->title->SetFont(FF_ARIAL, FS_NORMAL, 10);
    
    // Y-Axis
    $graph->yaxis->SetTitle($unit, 'middle');
    $graph->yaxis->title->SetFont(FF_ARIAL, FS_NORMAL, 10);

    // Use 20% "grace" to get slightly larger scale then min/max of
    // data
    $graph->yscale->SetGrace(0);
   
    $bar = new BarPlot($yAxis);

    $bar->SetValuePos('top');
    $bar->value->SetFont(FF_ARIAL);
    $bar->value->SetColor('black');
    $bar->value->SetFormat('%.1f °C');
    
    // print_r2($bar);
    
    $graph->Add($bar);
    $bar->value->Show();
    $graph->Stroke();
?>
