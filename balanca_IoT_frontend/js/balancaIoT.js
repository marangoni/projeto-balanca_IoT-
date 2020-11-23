var colectList = document.getElementById('colectList');
var localList = document.getElementById('localList');
var dataList = document.getElementById('dataList');
var pesoList = document.getElementById('pesoList');

const bal_database = {};
const med_database = {};

// //Acesso ao firebase para obtenção dos dados

//  firebase.database().ref('data').on('value', function (snapshot) {
//      colectList.innerHTML = '';
//      snapshot.forEach(function (item) {
//          var li = document.createElement('li');
//          li.appendChild(document.createTextNode(item.val().data_medida + ': ' + item.val().local + ': ' + item.val().massa));
//          colectList.appendChild(li);
//      });
//  });

(function(){

  let scaleId = false;

  // Funcao para adicionar uma nova balanca no DB
  function nova_balanca(apelido, local, material, dono,){
    
    if (!scaleId)
    scaleId = firebase.database().ref().child('balancas').push().key;

    const bal_data = {
      scaleId: scaleId ,
      apelido: apelido,
      local: local,
      material: material,
      dono: dono,
      createdat: firebase.database.ServerValue.TIMESTAMP,
      'Peso atual': "0,00 kg",
      'Valor atual': "R$ 0,00",
      'Ultima tara': "01/01/2020",
      'Ultimo esvaziamento': "01/01/2020",
      'Ultima atualizacao': "01/01/2020"
    };
      
    let updates = {};
    updates['/balancas/' + scaleId]=bal_data;
    
    let bal_ref = firebase.database().ref();

    bal_ref.update(updates)
      .then(function(){
        console.log("Balanca criada");
        return{ success: true, message: 'Balanca criada!'};
      })

      .catch(function(){
        console.log("Falha na criacao");
        return{ success: false, message: 'Falha na criacao'};
      })
    
  }

  // Funcao para remover uma nova balanca no DB
  function remove_bal(){
    if (!bal_id)
    return{ success: false, message: 'Invalid Scale!'};

    let bal_ref = firebase.database().ref('/balancas/' + bal_id);
    
    bal_ref.remove()
      .then(function(){
        console.log("Balanca removida");
        return{ success: true, message: 'Scale removed!'};
      })

      .catch(function(error){
        console.log("Erro na remocao");
        return{ success: false, message: 'Scale Remotion failed $(error.message'};
      });
  }

  // Funcao para detectar se houve uma nova medida no banco de dados
  // Ainda não funciona
  async function listen_med(){
    // if (!med_id)
    // return{ success: false, message: 'Invalid Game!'};

    let med_ref = firebase.database().ref('/medidas/');

    med_ref.once('child_added')
      .then(function(snapshot){
        // Peso
        if(snapshot.key == 'peso'){

            console.log('Peso changed!', snapshot.val());
            return{ success: true, message: 'Peso Updated!',data: snapshot.val()};
        // Gameover
        //   } else if (snapshot.key == 'gameover'){

        //   console.log('Game Over!', snapshot.val());
        //   return{ success: true, message: 'Game Over!',data: snapshot.val()};

        }
     
      })

      .catch(function(){
        return{ success: false, message: 'Invalid Data'};
      })

  }



    bal_database.new = nova_balanca;
    bal_database.remove = remove_bal;
  // med_database.update = update_game;
  // med_database.reset = reset_game;
     med_database.listen = listen_med;

})()
