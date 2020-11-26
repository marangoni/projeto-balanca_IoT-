var medsList = document.getElementById('medsList');
var medsTable= document.getElementById('medsTable');
var balList = document.getElementById('balList');
var valorPeso = document.getElementById('valorPeso');
var medidaEm = document.getElementById('medidaEm');
var balancaId = document.getElementById('balancaId');
var addButton = document.getElementById('addButton');

// Ao clicar no botão
addButton.addEventListener('click', function () {
    create(valorPeso.value, medidaEm.value, balancaId.value);
});

// Funcao para adicionar uma medida manualmente na tabela 'medidas'
function create(peso, medidaEm, balancaId){
    var data = {
        medidaEm: medidaEm,
        peso: peso,
        scaleId: balancaId 
    };
    return firebase.database().ref().child('medidas').push(data);
}

// Função geradora de uma listagem com as medidas no banco de dadosc
function mostrarMedidas (){
firebase.database().ref('medidas').on('value',function (snapshot) {
    medsList.innerHTML = '';
     snapshot.forEach(function (item) {
        var li = document.createElement('li');
        li.appendChild(document.createTextNode(item.val().medidaEm + ': ' 
         + item.val().peso + ': ' + item.val().scaleId));
        medsList.appendChild(li);
        }); 
    });
}

// Função geradora de uma listagem com as balancas no banco de dados
 function mostrarLixeiras (){
    firebase.database().ref('balancas').on('value',function (snapshot) {
        balList.innerHTML = '';
         snapshot.forEach(function (item) {
            var li = document.createElement('li');
            li.appendChild(document.createTextNode(item.val().local + ': ' 
             + item.val().apelido + ': ' + item.val().dono));
            balList.appendChild(li);
            }); 
        });
    }