/** @type {import('tailwindcss').Config} */
module.exports = {
  content: ["./assets/*.{html,js}"],
  theme: {
    extend: {
      screens: {
        'half': { 'raw': '(max-width: 1200px)' }, /* this is here because the default tailwind way is if screen px is above then it applies but that's silly, we want if screen is below then it applies */
      },
    },
  },
  plugins: [],
}
