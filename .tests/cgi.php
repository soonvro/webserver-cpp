<?php
// query string에서 'name' 매개변수 읽어오기
if (isset($_GET['name'])) {
    $name = $_GET['name'];
    echo '이름: ' . $name . '<br>';
} else {
    echo '이름이 지정되지 않았습니다.<br>';
}
if (isset($_SERVER['QUERY_STRING'])) {
  $name = $_SERVER['QUERY_STRING'];
  echo '이름: ' . $name . '<br>';
} else {
  echo '이름이 지정되지 않았습니다.<br>';
}
// query string에서 'age' 매개변수 읽어오기
if (isset($_GET['age'])) {
    $age = $_GET['age'];
    echo '나이: ' . $age;
} else {
    echo '나이가 지정되지 않았습니다.';
}



?>