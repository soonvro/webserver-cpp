<?php
  if (isset($_GET['name'])){
    $name = $_GET['name'];
    echo 'name: ' . $name . '<br>';
  } else {
    echo '@@@@@@@@@ <br>';

  }
?>