<?php
    //!
    //! @file Smarthome.php
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

    error_reporting(E_ALL);
    ini_set('display_errors','stdout');
    
    //!
    //! @name MySQL specific definitions
    //!
    //!@{

    $dbHost = "localhost";              //!< name of the MySQL server
    $db     = "Smarthome";              //!< database name
    $dbUser = "***";                    //!< database user, to be defined in Confidential.php
    $dbPw   = "***";                    //!< database password, to be defined in Confidential.php

    //!@}

    // define confidential db Access values here
    require_once ('Confidential.php');
    
    //!
    //! @name reporting function
    //!
    //! Prints out the passed value in html format to STDOUT
    //!
    
    function print_r2($val){
        echo '<pre>';
        print_r($val);
        echo  '</pre>';
    }
?>