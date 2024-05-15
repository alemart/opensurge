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
window.addEventListener('load', function() {
    const glider = new Glider(document.querySelector('.glider'), {
        slidesToShow: 1,
        dots: '#dots',
        draggable: true,
        arrows: {
            prev: '.glider-prev',
            next: '.glider-next'
        }
    });
});