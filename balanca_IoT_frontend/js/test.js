//Script de testes de acesso ao banco de dados

var colectList = document.getElementById('colectList');
var localList = document.getElementById('localList');
var dataList = document.getElementById('dataList');
var pesoList = document.getElementById('pesoList');

//var userId = firebase.auth().currentUser.uid;
var database = firebase.database().ref('/medidas/');
alert(database);
//console.log(database);

//Acesso ao firebase para obtenção dos dados

  firebase.database().ref('/medidas/').on('value', function (snapshot) {
      colectList.innerHTML = '';
      snapshot.forEach(function (item) {
          var li = document.createElement('li');
          li.appendChild(document.createTextNode(item.val().data_medida + ': ' + item.val().local + ': ' + item.val().massa));
          colectList.appendChild(li);
      });
  });