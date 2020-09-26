/** changes the type of the input item from passwrd to text
 * param input: the id of the input element
 * param status: the id of the status element. Currently not used. **/
function viewPassword(input, status) {
    var passwordInput = document.getElementById(input);
    // var passStatus = document.getElementById(status);
  
    if (passwordInput.type == 'password'){
        passwordInput.type='text';
        // passStatus.className='fa fa-eye-slash';
    } else {
        passwordInput.type='password';
        // passStatus.className='fa fa-eye';
    }
    passwordInput.focus();
}

/** Switch visiblility of elements with given class.
 * param e: true or false
 * param cls: the name of the class. **/
function vsw(e, cls){
    var t;
    t=!e?'none':'block';
    for(const n of document.getElementsByClassName(cls)) {
      n.style.display=t;
      input = n.getElementsByTagName('input')[0];
      if (input != undefined) {
        input.disabled=!e;
      }
    }
}
