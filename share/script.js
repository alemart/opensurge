// BE GREAT AGAIN! website
// Copyright (C) 2017 grey labs. All rights reserved.

$(function() {
    // Subscription form
    $('form').on('submit', function(e) {
        var email = $(this).find('input[type="email"]');
        var submit = $(this).find('button[type="submit"]');
        var response = $(this).find('.response');
        
        function disableButton() {
            submit.attr('disabled', 'disabled');
            submit.find('i.fa').removeClass('hidden');           
        }
        
        function enableButton() {
            submit.removeAttr('disabled');
            submit.find('i.fa').addClass('hidden');
        }
        
        function respond(text, icon) {
            response.find('i.fa').addClass('hidden');
            response.find('.response-text').text(text);
            response.find('.response-icon-' + icon).removeClass('hidden');
            response.removeClass('hidden');
            console.log(text);
        }

        e.preventDefault();
        disableButton();
        response.addClass('hidden');

        var rgx = /^\s*$/;
        if(!rgx.test(email.val())) {
            subscribe(email.val()).done(function(res) {
                enableButton();
                if(res.type == 'success') {
                    respond('Thank you! We\'ll keep you updated!', 'success');
                }
                else {
                    respond('Error submitting form. ' + res.data, 'error');
                    console.log('Erro ao cadastrar usuario (BGA)', res.data || res);
                }
            }).fail(function(jqxhr, status) {
                enableButton();
                respond('Error submitting form. HTTP Error.', 'error');
                console.log('Erro ao cadastrar usuario (BGA) [http]', status);
            });
        }
        else {
            enableButton();
            respond('Please type your e-mail!', 'error');
        }
        
        return false;
    });

    function subscribe(email) {
        return $.ajax({
            method: 'POST',
            url: '/be-great-again/subscribe',
            data: { 'email': email },
        });
    }

    // Slider & contact information
    var slider = $('.bxslider').bxSlider();
    var sliderTimer = setInterval(function() { slider.goToNextSlide(); }, 4000);
    $('.bx-pager-link').on('click touchstart', function() { clearInterval(sliderTimer); });
    $('#contact-info').attr('href', ('mai') + 'lto' + ':' + 'contato' + '@' + 'greylabs.com.br');
});