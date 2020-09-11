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
}