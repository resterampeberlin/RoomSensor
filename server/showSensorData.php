<?php
    //!
    //! @file showSensorData.php
    //! @author Markus Nickels
    //! @version 0.0
    //!
    //! This script shows the sensor data as a diagram
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
    require_once ('../jpgraph/src/jpgraph_line.php');
    require_once ('../jpgraph/src/jpgraph_date.php');
    require_once ('../jpgraph/src/jpgraph_error.php');
    
    //
    // Main
    //
    
    $xAxis = array();
    $yAxis = array();
    
    // Basic constants for display
    //print_r2($_GET);
    
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
    
    // sensorType to be displayed
    isset($_GET['sensorTypeId']) ?
            $sensorTypeId = $_GET['sensorTypeId'] :
            $sensorTypeId = '1';
    
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
    
    // Retrieve available sensors
    $stmt = $db->prepare('SELECT sensorData.sensorId, '.
                            'sensor.name,'.
                            'sensorType.unit,'.
                            'sensorType.name AS class '.
                         'FROM sensorData '.
                         'JOIN sensor ON sensorData.sensorId=sensor.id '.
                         'JOIN sensorType ON sensorType.Id=sensorTypeId '.
                         'WHERE sensorData.locationId=? '.
                            'AND sensorTypeId=? '.
                            'AND time BETWEEN ? AND ? '.
                         'GROUP BY sensorId;');
    $stmt->execute(array($locationId,
                         $sensorTypeId,
                         $dateFrom->format(DateTime::W3C),
                         $dateTo->format(DateTime::W3C)));
    
    if ($stmt->rowCount() == 0) {
        echo 'No data found for '.$locationName.'<br>';
        exit(1);
    }
    
    while($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        // print_r2($row);
        
        $sensors[$row['sensorId']] =  $row['name'];
        $class = $row['class'];
        $unit = $row['unit'];
    }
    
    // Get Data
    $stmt = $db->prepare('SELECT * '.
                         'FROM sensorData '.
                         'JOIN sensor ON sensorData.sensorId=sensor.Id '.
                         'WHERE locationId=? '.
                            'AND sensorTypeId=? '.
                            'AND time BETWEEN ? AND ? '.
                         'ORDER BY time ASC;');
    $stmt->execute(array($locationId,
                         $sensorTypeId,
                         $dateFrom->format(DateTime::W3C),
                         $dateTo->format(DateTime::W3C)));
    
    // Initialise x-axis
    $i = 0;
    $daterange = new DatePeriod($dateFrom, $delta ,$dateTo);
    
    foreach($daterange as $date){
        $xAxis[$i++] = $date->getTimeStamp();
    }
    
    // initialise y-axis
    foreach($sensors as $key => $value) {
        $yAxis[$key][0] = '';
    }

    $i = 0;
    $minValue = 100;
    $maxValue = 0;
    
    while($stmt && $row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        // print_r2($row);
        
        $timeStamp = new DateTime($row['time']);
        
        // advance to matching time
        while($i < count($xAxis)-1 &&
              $timeStamp->getTimeStamp() >= $xAxis[$i+1]) {
            foreach($sensors as $key => $value) {
                $yAxis[$key][$i+1] = $yAxis[$key][$i];
            }
            $i++;
        }
        
        $yAxis[$row['sensorId']][$i] = $row['value'];
        
        $minValue = min($minValue, $row['value']);
        $maxValue = max($maxValue, $row['value']);
    }
    
    // fill up with empty value
    while($i < count($xAxis)-1) {
        foreach($sensors as $key => $value) {
            $yAxis[$key][$i+1] = '';
        }
        $i++;
    }
    
    // print_r2($xAxis);
    // print_r2($yAxis);
    // print_r2($unit);
    // print_r2($minValue);
    // print_r2($maxValue);
    
    $graph = new Graph(1200,500);
    $graph->img->SetMargin(40,40,40,40);
    $graph->img->SetAntiAliasing();
    $graph->SetScale('dateint', $minValue, $maxValue);
    $graph->SetShadow();
    
    // Title
    $graph->title->Set($class.' '.$locationName);
    $graph->title->SetFont(FF_ARIAL, FS_BOLD, 12);
    $graph->subtitle->SetFont(FF_ARIAL, FS_NORMAL, 10);
    $graph->subtitle->Set($dateFrom->format('d.m.y H:i:s').
                          ' bis '.
                          $dateTo->format('d.m.y H:i:s'));
    
    // X-Axis
    // $graph->xaxis->SetTickLabels($xAxis);
    $graph->xaxis->SetLabelAngle(90);
    $graph->xaxis->scale->SetDateFormat('H:i');
    $graph->xaxis->scale->ticks->Set(60*60);
    $graph->xaxis->scale->SetTimeAlign(MINADJ_1);
    
    // Y-Scale
    // Use 20% "grace" to get slightly larger scale then min/max of
    // data
    $graph->yscale->SetGrace(0);

    // Y-Axis
    $graph->yaxis->SetTitle($unit, 'middle');
    $graph->yaxis->title->SetFont(FF_ARIAL, FS_NORMAL, 10);
    $graph->yaxis->SetLabelFormat('%0.1f');

    foreach($sensors as $key => $value) {
        $p1 = new LinePlot($yAxis[$key], $xAxis);
        $p1->SetStepStyle();
        $p1->mark->SetWidth(4);
        $p1->SetColor("blue");
        $p1->SetCenter();
        $p1->SetLegend($value);
        
        $graph->Add($p1);
    }
    
    $graph->Stroke();
?>
