var medsList = document.getElementById('medsList');
var medsTable= document.getElementById('medsTable');
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

// Função geradora de uma listagem com as medidas no banco de dados
mostraMedidas = function (){
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

/* // Função geradora de uma tabela com as medidas no banco de dados
firebase.database().ref('medidas').on('value',function (snapshot) {
    medsTable.innerHTML = '';
     snapshot.forEach(function (item) {
        var tr = document.createElement('TABLE');
        tr.appendChild(document.createTextNode(
        <td> item.val().medidaEm) </td>
         + <td> item.val().peso </td> 
         + <td> item.val().scaleId </td>
         ));
        medsTable.appendChild(tr);
    }); 
}); */


{/* <tbody>
        <tr class="table-active">
          <th scope="row">Active</th>
          <td>Column content</td>
          <td>Column content</td>
        </tr> */}