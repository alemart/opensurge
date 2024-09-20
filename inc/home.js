/* reveal on scroll */
window.addEventListener('load', function() {
    if(typeof IntersectionObserver === 'undefined')
        return;

    const observer = new IntersectionObserver(function(entries) {
        for(const entry of entries) {
            if(entry.intersectionRatio >= 0.5)
                entry.target.classList.remove('unrevealed');
            else if(entry.intersectionRatio == 0.0)
                entry.target.classList.add('unrevealed');
        }
    }, { threshold: [0, 0.5] });

    document.querySelectorAll('.reveal').forEach(function(reveal) {
        observer.observe(reveal);
    });
});

/* carousel */
document.addEventListener('DOMContentLoaded', function() {
    const glider = new Glider(document.querySelector('#testimonials .glider'), {
        slidesToShow: 1,
        //dots: '#testimonials .dots',
        draggable: true,
        arrows: {
            prev: '#testimonials .glider-prev',
            next: '#testimonials .glider-next'
        }
    });
});

/* showcase */
document.addEventListener('DOMContentLoaded', function() {
    function shuffle(arr) {
        let j = arr.length;
        while(j > 0) {
            let i = Math.floor(Math.random() * j--);
            [arr[i], arr[j]] = [arr[j], arr[i]];
        }
        return arr;
    }

    const set = document.querySelectorAll('#games .game');
    const seq = Array.from({ length: set.length }, (_, i) => i);
    const ord = shuffle(seq);
    set.forEach((game, i) => game.style.setProperty('order', ord[i]));
});