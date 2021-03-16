(function ()
{

	var fsicon = document.createElement('img');

	var srcFSI = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAC4AAAAvCAYAAACc5fiSAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsIAAA7CARUoSoAAAAKJSURBVGhDxZq/rgFBFIeXeAyJF6BAJVQeQOJBdCq9WuctRKJSKFdEgYJOJfEee+9vMnMjY2b3/NnLl0hmNjNnvhxj5hCVxNJoNDLbZPF4PP5icFCvNxwOMw1pmrIFMEcDnJPj8Wi7cnq9HlkeY7XAuWrjqTgcDuTtwhmbh1q80+nYFh3JHB+VOAQulws7g5ijlReLS6UdWnmRuFbaoZFniy+Xy1KkHYiFmGwkx+F8PhddHiEQiwucxed4GfISaaASBxp5qTRQiwOJvEYaFIoPBoNst9vZXhyOPEUaa2LtGLni7Xb7T4byrlDkKdJGygKHEFHxUNFEqejy5CnSoUozVJQZcX8rvGba53Q62VFxQvIUacS2w9/w5bfbbWYKeky63++50g5u5qWZ9oEbHJFt6ZcQ0p6fTqfmVYR52z8JRb6Ij0s7NPJfk3ZQPrA+eR/Ej8LJfFmZLq08hZRt5lL5xTZVqL9zgtlsRs4iZ+y/AhEuX5eXSDu+Jq+RdnxcniKNGohSEqvkJ5MJuaamSL+e05RznioPR7iazmKxsNMzU8SYhxEo0tfr9S0GnhVRJA83B5zfMhKT52baR5P5V2lg1gnder68Vtohkfelgbl9Y9e1k6eUpqHtEYOybbAmxoakQa44FUqmfSiZzwPOqiv/drsl3W6XXXtgDuZqEIufz+ek1WqJCybM1ciLxKWZ9tHIs8X3+70q0z6IhZhcSilrvwFbvN/vmyPNdtUgFmJyEWW82WyWIo8YiCVBvFW08hppoNrjUnmtNKjWajXblAEB/CRsu4VgrFYaztXVamW7ctI0JR+PnLEx4GyCjMfjrF6vm4dcNpsN+x8U+NFyNBrZHo/n85ms1+vKD7Xnlaru15bbAAAAAElFTkSuQmCC';
	var srcFSIexit = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAC4AAAAuCAYAAABXuSs3AAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsIAAA7CARUoSoAAAAK4SURBVGhD7Zkxs/FAFIbjK1Qav4CexqjUCqPUaTVUOhW1GZ2KjtpPUBiVhhkU6Kn8AJ3GnZPvXZNszsrJunco8jQ3Z7PneO6ZSFbW8VKtVh+5XO6BUEStVvvT+US73TbnkLQik8mIipMEgVDEdDp9DIdDcc52u3U/o9/vB3O80ooweSVNYEjEaDRycyTySlrhk+ekFSZ5rzSBYRFKnBgMBsbc9XqNWX663e7/nOv1iiEeXV6XJnBKhFec4Dqvd1oH08InKnlOmnCLCNHFCa+81OVJWEKr1cJREJQQwYkTdP3O53NEPMbvXZi8CaSLMImHoUv/w1+XYrGY2O12iL6HbDbrXC6XBEIXnzjxbfKcNBEQJ0j+eDwi+hwmaYIVp3t7Pp9H9DmazSaOBLx6IJlAqoioX85XD6knNtIE0kXY3FXYdYrCVppACRG2t0O28+9IEygjwlac8HW+Xq9j2B6UEkHL2negf5zqJKjbt9vNLWrLarVib1kc9EMlnU4jik4ymXSWy6X482JiYmJiYmKIRKFQeNzvd4R2nE4n8SOYfvSmUilE0aFH/n6/T7gvFN8FNUW8u8iazWa/t0JEGRHvLGvVytBHo9HA6eighAhb8clkYv4cW3mki7ARZzutY3PZIFVEVPGXndaJ2nmkiYgiLuq0zuFwQHo4SBERRRwpLOwLocVi8RUvhF7JB8RJulwuI/o8JnmfOL2f/iZpBSf/FKdOVyoVREF6vR6O/obxeOxsNhtEQdjOh+0EqL1P093GLSKE+3LSMgCnnfP5jFEeTDPvbiloEYapLpw8TonQxb3SCpE8bb+ZMO0y6/IYFuEV56QVJnm6OjCFl9c7rePdzMKQCCXuW+UZ0LcyfdKKTqeD0+ZO66jOIxRBXZZIK1TnWWkFdb5UKkUSoc7jUAT9szgUE7ykHOcHENJwAZYVRpwAAAAASUVORK5CYII=';
	fsicon.src = srcFSI;
	fsicon.id = 'fsicon';

	fsicon.style.opacity = 0.5;
	fsicon.style.filter = 'alpha( opacity=50 )';
	fsicon.style.cursor = 'pointer';
	fsicon.style.zIndex = 2000;
	fsicon.style.top = '10px';
	fsicon.style.right = '10px';
	fsicon.style.position = 'fixed';

	document.body.appendChild(fsicon);

	var fsicon = document.getElementById("fsicon");

	if (fsicon)
	{
		fsicon.addEventListener("click", function ()
		{
			if (fsicon.getAttribute('src') != srcFSIexit)
			{
				var docElm = document.documentElement;
				if (docElm.requestFullscreen)
				{
					docElm.requestFullscreen();
				}
				else if (docElm.mozRequestFullScreen)
				{
					docElm.mozRequestFullScreen();
				}
				else if (docElm.webkitRequestFullScreen)
				{
					docElm.webkitRequestFullScreen();
				}
			} else
			{
				if (document.exitFullscreen)
				{
					document.exitFullscreen();
				}
				else if (document.mozCancelFullScreen)
				{
					document.mozCancelFullScreen();
				}
				else if (document.webkitCancelFullScreen)
				{
					document.webkitCancelFullScreen();
				}
			}

		}, false);
	}
	if (fsicon)
	{
		document.addEventListener("fullscreenchange", function ()
		{
			if (document.fullscreenElement)
			{
				fsicon.setAttribute('src', srcFSIexit);
			} else
			{
				fsicon.setAttribute('src', srcFSI);
			};

		}, false);

		document.addEventListener("mozfullscreenchange", function ()
		{
			if (document.mozFullScreen)
			{
				fsicon.setAttribute('src', srcFSIexit);
			} else
			{
				fsicon.setAttribute('src', srcFSI);
			};
		}, false);

		document.addEventListener("webkitfullscreenchange", function ()
		{
			if (document.webkitIsFullScreen)
			{
				fsicon.setAttribute('src', srcFSIexit);
			} else
			{
				fsicon.setAttribute('src', srcFSI);
			};
		}, false);
	}

})();